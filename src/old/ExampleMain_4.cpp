#include <GigaLearnCPP/Learner.h>

#include <RLGymCPP/Rewards/CommonRewards.h>
#include <RLGymCPP/Rewards/ZeroSumReward.h>
#include <RLGymCPP/TerminalConditions/NoTouchCondition.h>
#include <RLGymCPP/TerminalConditions/GoalScoreCondition.h>
#include <RLGymCPP/OBSBuilders/AdvancedObs.h>
#include <RLGymCPP/StateSetters/KickoffState.h>
#include <RLGymCPP/ActionParsers/DefaultAction.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace GGL; // GigaLearn
using namespace RLGC; // RLGymCPP

// ============================================================================
// CUSTOM 2V2 STRATEGY REWARDS
// ============================================================================

// CRITICAL: Shoot in the RIGHT net (not own goal!)
class CorrectGoalReward : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		if (player.ballTouchedStep) {
			bool isBlue = (player.team == Team::BLUE);
			Vec correctGoal = isBlue ? Vec(0, 5120, 0) : Vec(0, -5120, 0);  // Blue shoots at +Y
			Vec wrongGoal = isBlue ? Vec(0, -5120, 0) : Vec(0, 5120, 0);
			
			Vec ballToCorrectGoal = (correctGoal - state.ball.pos).Normalized();
			Vec ballToWrongGoal = (wrongGoal - state.ball.pos).Normalized();
			
			float correctAlignment = state.ball.vel.Normalized().Dot(ballToCorrectGoal);
			float wrongAlignment = state.ball.vel.Normalized().Dot(ballToWrongGoal);
			
			// Reward hitting toward correct goal
			if (correctAlignment > 0.5f) {
				return 2.0f;
			}
			
			// PUNISH hitting toward own goal
			if (wrongAlignment > 0.5f) {
				return -5.0f;  // BIG PENALTY for own goals
			}
		}
		return 0.0f;
	}
};

// Keep ball low in opponent half (no random booms)
class LowShotReward : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		if (player.ballTouchedStep) {
			bool isBlue = (player.team == Team::BLUE);
			float ballY = state.ball.pos.y;
			
			// In opponent half
			bool inOpponentHalf = isBlue ? (ballY > 0) : (ballY < 0);
			
			if (inOpponentHalf) {
				float ballHeight = state.ball.pos.z;
				
				// Reward low shots (under 300uu)
				if (ballHeight < 300 && state.ball.vel.Length() > 1000) {
					return 1.0f;  // Good low shot
				}
			}
		}
		return 0.0f;
	}
};

// NO DOUBLE COMMIT - if teammate closer, stay back
class NoDoubleCommitReward : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		float myDistToBall = (player.pos - state.ball.pos).Length();
		
		// Find closest teammate distance
		float teammateClosestDist = 999999.0f;
		for (const auto& p : state.players) {
			if (p.team == player.team && p.carId != player.carId) {
				float dist = (p.pos - state.ball.pos).Length();
				if (dist < teammateClosestDist) {
					teammateClosestDist = dist;
				}
			}
		}
		
		// If teammate much closer, I should NOT be near ball
		if (teammateClosestDist < myDistToBall - 800) {
			// I'm further, so stay back
			if (myDistToBall > 2500) {
				return 1.0f;  // Good, staying back
			}
			
			// Too close = double commit
			if (myDistToBall < 1500) {
				return -1.0f;  // PENALTY for double commit
			}
		}
		
		return 0.0f;
	}
};

// Maintain distance from teammate (spread out)
class TeamSpacingReward : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		for (const auto& p : state.players) {
			if (p.team == player.team && p.carId != player.carId) {
				float spacing = (player.pos - p.pos).Length();
				
				// Good spacing (2000-4000 units apart)
				if (spacing > 2000 && spacing < 4000) {
					return 0.5f;
				}
				
				// Too close = bad
				if (spacing < 1000) {
					return -0.5f;
				}
			}
		}
		return 0.0f;
	}
};

// Be on defensive side when not attacking
class DefensivePositioningReward : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		bool isBlue = (player.team == Team::BLUE);
		Vec myGoal = isBlue ? Vec(0, -5120, 0) : Vec(0, 5120, 0);
		
		float myDistToBall = (player.pos - state.ball.pos).Length();
		float myDistToGoal = (player.pos - myGoal).Length();
		
		// Find if teammate is closer to ball
		bool teammateCloser = false;
		for (const auto& p : state.players) {
			if (p.team == player.team && p.carId != player.carId) {
				float teammateDistToBall = (p.pos - state.ball.pos).Length();
				if (teammateDistToBall < myDistToBall) {
					teammateCloser = true;
				}
			}
		}
		
		// If teammate closer (they're attacking), I should be defensive
		if (teammateCloser) {
			// Reward being between ball and goal
			Vec ballToGoal = (myGoal - state.ball.pos).Normalized();
			Vec ballToMe = (player.pos - state.ball.pos).Normalized();
			float alignment = ballToGoal.Dot(ballToMe);
			
			// Behind ball = good
			if (alignment > 0.3f && myDistToGoal < 3500) {
				return 1.0f;
			}
		}
		
		return 0.0f;
	}
};

// Be on correct side of field (if on right side, be on right)
class FieldSideReward : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		float ballX = state.ball.pos.x;
		float myX = player.pos.x;
		float myDistToBall = (player.pos - state.ball.pos).Length();
		
		// Find if teammate is closer
		bool teammateCloser = false;
		for (const auto& p : state.players) {
			if (p.team == player.team && p.carId != player.carId) {
				if ((p.pos - state.ball.pos).Length() < myDistToBall) {
					teammateCloser = true;
				}
			}
		}
		
		// If teammate attacking, I should mirror the ball's side
		if (teammateCloser && myDistToBall > 2000) {
			// Ball on right, I should be on right side too (covering)
			if ((ballX > 500 && myX > 0) || (ballX < -500 && myX < 0)) {
				return 0.3f;
			}
		}
		
		return 0.0f;
	}
};

// ============================================================================
// FRESH START - 2V2 BOT TRAINING
// Goals: Score, Rotate, Aerials, Boost Management, NO FLIPPING
// ============================================================================

EnvCreateResult EnvCreateFunc(int index) {
	std::vector<WeightedReward> rewards = {
		// ====================================================================
		// #1 PRIORITY: SCORE GOALS (RIGHT NET!)
		// ====================================================================
		{ new GoalReward(), 1000 },                                         // MASSIVE
		{ new CorrectGoalReward(), 20.0f },                                 // Shoot right way!
		{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 100.0f },  // Shoot on target
		{ new StrongTouchReward(20, 150), 150 },                            // Powerful hits
		{ new LowShotReward(), 10.0f },                                     // Keep ball low in opp half
		
		// ====================================================================
		// #2 PRIORITY: 2V2 STRATEGY (NO DOUBLE COMMIT!)
		// ====================================================================
		{ new NoDoubleCommitReward(), 15.0f },                              // HIGH - stay back if teammate closer
		{ new TeamSpacingReward(), 10.0f },                                 // Keep distance from teammate
		{ new DefensivePositioningReward(), 12.0f },                        // Be defensive when teammate attacks
		{ new FieldSideReward(), 8.0f },                                    // Be on correct side of field
		
		// ====================================================================
		// #3 PRIORITY: AERIALS
		// ====================================================================
		{ new AirReward(), 10.0f },                                         // Be in air
		
		// ====================================================================
		// #4 PRIORITY: BOOST MANAGEMENT
		// ====================================================================
		{ new PickupBoostReward(), 8.f },                                   // Get boost pads
		{ new SaveBoostReward(), 5.0f },                                    // Don't waste boost
		
		// ====================================================================
		// BALL PLAY (MODERATE - go for ball but not too much)
		// ====================================================================
		{ new VelocityPlayerToBallReward(), 2.0f },                         // Go toward ball (moderate)
		{ new FaceBallReward(), 0.5f },                                     // Face ball (low)
		
		// ====================================================================
		// COMPETITIVE
		// ====================================================================
		{ new ZeroSumReward(new BumpReward(), 0.5f), 20 },
		{ new ZeroSumReward(new DemoReward(), 0.5f), 60 }
	};
	
	std::vector<TerminalCondition*> terminalConditions = {
		new NoTouchCondition(12),    // 12 seconds no touch = reset
		new GoalScoreCondition()     // Goal = reset
	};

	// 2v2 arena
	int playersPerTeam = 2;
	auto arena = Arena::Create(GameMode::SOCCAR);
	for (int i = 0; i < playersPerTeam; i++) {
		arena->AddCar(Team::BLUE);
		arena->AddCar(Team::ORANGE);
	}

	EnvCreateResult result = {};
	result.actionParser = new DefaultAction();
	result.obsBuilder = new AdvancedObs();
	result.stateSetter = new KickoffState();
	result.terminalConditions = terminalConditions;
	result.rewards = rewards;
	result.arena = arena;

	return result;
}

void StepCallback(Learner* learner, const std::vector<GameState>& states, Report& report) {
	bool doExpensiveMetrics = (rand() % 4) == 0;

	for (auto& state : states) {
		if (doExpensiveMetrics) {
			for (auto& player : state.players) {
				report.AddAvg("Player/In Air Ratio", !player.isOnGround);
				report.AddAvg("Player/Ball Touch Ratio", player.ballTouchedStep);
				report.AddAvg("Player/Speed", player.vel.Length());
				report.AddAvg("Player/Boost", player.boost);
				
				if (player.ballTouchedStep)
					report.AddAvg("Player/Touch Height", state.ball.pos.z);
			}
		}
		if (state.goalScored)
			report.AddAvg("Game/Goal Speed", state.ball.vel.Length());
	}
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char* argv[]) {
#ifdef _WIN32
	_putenv("PYTHONPATH=C:\\Users\\Jake\\Videos\\Jake\\GigaLearnCPP-Leak-main");
#endif

	RocketSim::Init("C:\\Users\\Jake\\Videos\\Jake\\GigaLearnCPP-Leak-main\\collision_meshes");

	std::cout << "========================================" << std::endl;
	std::cout << "FRESH START - 2v2 Bot Training" << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << "Goals:" << std::endl;
	std::cout << "  • Score goals RIGHT NET (1000 pts)" << std::endl;
	std::cout << "  • NO double commit (-1.0 penalty)" << std::endl;
	std::cout << "  • Keep distance from teammate" << std::endl;
	std::cout << "  • Defensive positioning when needed" << std::endl;
	std::cout << "  • Be on correct side of field" << std::endl;
	std::cout << "  • Keep ball low in opponent half" << std::endl;
	std::cout << "  • Hit aerials (10 pts)" << std::endl;
	std::cout << "  • Powerful shots (150 pts)" << std::endl;
	std::cout << "  • Boost management" << std::endl;
	std::cout << "========================================\n" << std::endl;

	LearnerConfig cfg = {};
	cfg.deviceType = LearnerDeviceType::GPU_CUDA;
	cfg.tickSkip = 4;
	cfg.actionDelay = cfg.tickSkip - 1;
	cfg.numGames = 256;
	cfg.randomSeed = 123;

	// Training hyperparameters
	int tsPerItr = 90'000;
	cfg.ppo.tsPerItr = tsPerItr;
	cfg.ppo.batchSize = tsPerItr;
	cfg.ppo.miniBatchSize = 90'000;
	cfg.ppo.epochs = 1;
	cfg.ppo.entropyScale = 0.035f;      // Encourages exploration
	cfg.ppo.gaeGamma = 0.99;
	cfg.ppo.policyLR = 3e-4f;           // Standard learning rate
	cfg.ppo.criticLR = 3e-4f;

	// Model architecture - 256 (good balance)
	cfg.ppo.sharedHead.layerSizes = { 256, 256 };
	cfg.ppo.policy.layerSizes = { 256, 256, 256 };
	cfg.ppo.critic.layerSizes = { 256, 256, 256 };

	auto optim = ModelOptimType::ADAM;
	cfg.ppo.policy.optimType = optim;
	cfg.ppo.critic.optimType = optim;
	cfg.ppo.sharedHead.optimType = optim;

	auto activation = ModelActivationType::RELU;
	cfg.ppo.policy.activationType = activation;
	cfg.ppo.critic.activationType = activation;
	cfg.ppo.sharedHead.activationType = activation;

	cfg.ppo.policy.addLayerNorm = true;
	cfg.ppo.critic.addLayerNorm = true;
	cfg.ppo.sharedHead.addLayerNorm = true;

	// Metrics and rendering
	cfg.sendMetrics = true;
	cfg.renderMode = true;

	std::cout << "Starting fresh training from scratch..." << std::endl;
	std::cout << "Expected timeline:" << std::endl;
	std::cout << "  100M steps: Basic ball contact" << std::endl;
	std::cout << "  300M steps: Shooting on goal" << std::endl;
	std::cout << "  500M steps: Aerials starting" << std::endl;
	std::cout << "  1B+ steps: Advanced play\n" << std::endl;

	Learner* learner = new Learner(EnvCreateFunc, cfg, StepCallback);
	learner->Start();

	delete learner;
	
	std::cout << "\n✓ TRAINING COMPLETE!" << std::endl;
	return EXIT_SUCCESS;
}
