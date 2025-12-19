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
// ANTI-FLIP REWARDS
// ============================================================================

// Reward staying on ground and driving normally
class GroundDrivingReward : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		// Reward being on ground and moving
		if (player.isOnGround && player.vel.Length() > 500) {
			return 0.5f;  // Continuous reward for normal driving
		}
		return 0.0f;
	}
};

// DIRECT PENALTY for excessive flipping
class FlipPenalty : public Reward {
private:
	std::map<int, bool> wasOnGround;
	std::map<int, int> flipCount;
	std::map<int, int> framesSinceFlip;
	
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		bool onGround = player.isOnGround;
		int carId = player.carId;
		
		// Detect flip: was on ground, now in air with rotation
		if (wasOnGround[carId] && !onGround) {
			float rotSpeed = player.angVel.Length();
			
			// High rotation speed = probably flipped
			if (rotSpeed > 3.0f) {
				flipCount[carId]++;
				framesSinceFlip[carId] = 0;
				
				float distToBall = (player.pos - state.ball.pos).Length();
				
				// Flipping far from ball = BIG PENALTY
				if (distToBall > 1500) {
					return -2.0f;  // HARSH PENALTY
				}
				// Flipping close to ball = smaller penalty (might be useful)
				else if (distToBall > 500) {
					return -0.5f;  // Small penalty
				}
			}
		}
		
		wasOnGround[carId] = onGround;
		framesSinceFlip[carId]++;
		
		return 0.0f;
	}
};

// ============================================================================
// ORIGINAL STAGE 4 - ANTI-FLIP VERSION
// ============================================================================

EnvCreateResult EnvCreateFunc(int index) {
	// EXACT Stage 4 from your original config
	std::vector<WeightedReward> rewards = {
		{ new AirReward(), 15.0f },
		{ new StrongTouchReward(20, 150), 200 },
		{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 100.0f },
		
		// ONLY CHANGE: Reduced these two
		{ new VelocityPlayerToBallReward(), 1.0f },   // Was 5.f
		{ new FaceBallReward(), 0.2f },               // Was 1.0f
		
		{ new PickupBoostReward(), 12.f },
		{ new SaveBoostReward(), 3.0f },
		{ new GoalReward(), 500 },
		
		// ANTI-FLIP: Reward driving, PUNISH flipping
		{ new GroundDrivingReward(), 15.0f },         // HIGH - reward normal driving
		{ new FlipPenalty(), 10.0f }                  // HIGH - punish wasteful flips
	};
	
	std::vector<TerminalCondition*> terminalConditions = {
		new NoTouchCondition(10),
		new GoalScoreCondition()
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
	std::cout << "ORIGINAL Stage 4 - Anti-Ballchase" << std::endl;
	std::cout << "Back to what was working!" << std::endl;
	std::cout << "========================================\n" << std::endl;

	LearnerConfig cfg = {};
	cfg.deviceType = LearnerDeviceType::GPU_CUDA;
	cfg.tickSkip = 4;
	cfg.actionDelay = cfg.tickSkip - 1;
	cfg.numGames = 256;
	cfg.randomSeed = 123;

	int tsPerItr = 90'000;
	cfg.ppo.tsPerItr = tsPerItr;
	cfg.ppo.batchSize = tsPerItr;
	cfg.ppo.miniBatchSize = 90'000;
	cfg.ppo.epochs = 1;
	cfg.ppo.entropyScale = 0.035f;
	cfg.ppo.gaeGamma = 0.99;
	cfg.ppo.policyLR = 2e-4f;
	cfg.ppo.criticLR = 2e-4f;

	// Stage 4 architecture (256 to match checkpoint!)
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

	cfg.sendMetrics = true;
	cfg.renderMode = true;

	std::cout << "CHANGES from original:" << std::endl;
	std::cout << "  VelocityPlayerToBallReward: 5.0 → 1.0" << std::endl;
	std::cout << "  FaceBallReward: 1.0 → 0.2" << std::endl;
	std::cout << "  + GroundDrivingReward: 15.0" << std::endl;
	std::cout << "  + FlipPenalty: -2.0 (far from ball)" << std::endl;
	std::cout << "                 -0.5 (close to ball)\n" << std::endl;

	Learner* learner = new Learner(EnvCreateFunc, cfg, StepCallback);
	learner->Start();

	delete learner;
	
	std::cout << "\n✓ TRAINING COMPLETE!" << std::endl;
	return EXIT_SUCCESS;
}