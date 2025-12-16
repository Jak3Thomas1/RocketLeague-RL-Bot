#include <GigaLearnCPP/Learner.h>

#include <RLGymCPP/Rewards/CommonRewards.h>
#include <RLGymCPP/Rewards/ZeroSumReward.h>
#include <RLGymCPP/TerminalConditions/NoTouchCondition.h>
#include <RLGymCPP/TerminalConditions/GoalScoreCondition.h>
#include <RLGymCPP/OBSBuilders/DefaultObs.h>
#include <RLGymCPP/OBSBuilders/AdvancedObs.h>
#include <RLGymCPP/StateSetters/KickoffState.h>
#include <RLGymCPP/StateSetters/RandomState.h>
#include <RLGymCPP/ActionParsers/DefaultAction.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace GGL; // GigaLearn
using namespace RLGC; // RLGymCPP

// ============================================================================
// CURRICULUM TRAINING: 7-STAGE PRO-LEVEL 2V2 BOT
// With Hardcoded Speed Flip Kickoffs
// ============================================================================

// Current training stage
int currentStage = 7;

// Kickoff tick counter (tracks kickoff progress)
int kickoffTick = 0;
bool isKickoff = false;

// ============================================================================
// HARDCODED KICKOFF SEQUENCE (Speed Flip)
// Based on professional Rocket League kickoff timing
// ============================================================================
struct KickoffAction {
	float throttle = 0;
	float steer = 0;
	float pitch = 0;
	float yaw = 0;
	float roll = 0;
	bool jump = false;
	bool boost = false;
};

KickoffAction GetHardcodedKickoffAction(int tick) {
	KickoffAction action;
	
	// Ticks 0-43: Boost straight + diagonal (11*4 + 4*4 ticks)
	if (tick < 44) {
		action.throttle = 1.0f;
		action.boost = true;
		if (tick >= 11 * 4) {  // After straight boost, turn slightly
			action.steer = -0.3f;  // Slight diagonal
		}
	}
	// Ticks 44-51: First jump (2*4 ticks)
	else if (tick < 52) {
		action.throttle = 1.0f;
		action.jump = true;
		action.boost = true;
	}
	// Ticks 52-55: Brief coast (1*4 ticks)
	else if (tick < 56) {
		action.throttle = 1.0f;
		action.boost = true;
	}
	// Ticks 56-59: Speed flip (1*4 ticks)
	else if (tick < 60) {
		action.throttle = 1.0f;
		action.yaw = 0.8f;
		action.pitch = -0.7f;
		action.jump = true;
		action.boost = true;
	}
	// Ticks 60-111: Pitch up recovery (13*4 ticks)
	else if (tick < 112) {
		action.throttle = 1.0f;
		action.pitch = 1.0f;
		action.boost = true;
	}
	// Ticks 112-151: Air control (10*4 ticks)
	else if (tick < 152) {
		action.throttle = 1.0f;
		action.roll = 1.0f;
		action.pitch = 0.5f;
	}
	// After tick 152: Let RL take over
	else {
		// Return neutral action to signal RL should control
		action.throttle = 0;
	}
	
	return action;
}

// ============================================================================
// ENVIRONMENT CREATION WITH REWARDS PER STAGE
// ============================================================================
EnvCreateResult EnvCreateFunc(int index) {
	std::vector<WeightedReward> rewards;
	std::vector<TerminalCondition*> terminalConditions;
	
	// ========================================================================
	// STAGE 1: BALL CONTACT & AWARENESS (100M steps)
	// ========================================================================
	if (currentStage == 1) {
		rewards = {
			{ new StrongTouchReward(5, 50), 100 },
			{ new FaceBallReward(), 5.0f },
			{ new VelocityPlayerToBallReward(), 10.f },
			{ new PickupBoostReward(), 5.f },
			{ new GoalReward(), 200 }
		};
		terminalConditions = {
			new NoTouchCondition(15),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 2: GOAL SHOOTING (200M steps)
	// ========================================================================
	else if (currentStage == 2) {
		rewards = {
			{ new StrongTouchReward(5, 50), 30 },
                	{ new VelocityBallToGoalReward(), 80.0f }, // FIXED - Removed ZeroSum!
			{ new VelocityPlayerToBallReward(), 8.f },
			{ new FaceBallReward(), 2.0f },
			{ new PickupBoostReward(), 8.f },
			{ new SaveBoostReward(), 1.0f },
			{ new GoalReward(), 500 }
		};
		terminalConditions = {
			new NoTouchCondition(12),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 3: POWER & ACCURACY (300M steps)
	// ========================================================================
	else if (currentStage == 3) {
		rewards = {
			{ new StrongTouchReward(20, 150), 150 },
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 80.0f },
			{ new VelocityPlayerToBallReward(), 6.f },
			{ new FaceBallReward(), 1.5f },
			{ new PickupBoostReward(), 10.f },
			{ new SaveBoostReward(), 2.0f },
			{ new ZeroSumReward(new BumpReward(), 0.5f), 30 },
			{ new GoalReward(), 400 }
		};
		terminalConditions = {
			new NoTouchCondition(10),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 4: AERIAL FUNDAMENTALS (500M steps)
	// ========================================================================
	else if (currentStage == 4) {
		rewards = {
			{ new AirReward(), 15.0f },
			{ new StrongTouchReward(20, 150), 200 },
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 100.0f },
			{ new VelocityPlayerToBallReward(), 5.f },
			{ new FaceBallReward(), 1.0f },
			{ new PickupBoostReward(), 12.f },
			{ new SaveBoostReward(), 3.0f },
			{ new GoalReward(), 500 }
		};
		terminalConditions = {
			new NoTouchCondition(10),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 5: AIR DRIBBLES (600M steps)
	// ========================================================================
	else if (currentStage == 5) {
		rewards = {
			{ new AirReward(), 25.0f },
			{ new StrongTouchReward(30, 200), 300 },
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 120.0f },
			{ new VelocityPlayerToBallReward(), 8.f },
			{ new PickupBoostReward(), 15.f },
			{ new SaveBoostReward(), 4.0f },
			{ new GoalReward(), 600 }
		};
		terminalConditions = {
			new NoTouchCondition(10),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 6: DOUBLE TAPS & WALL PLAY (600M steps)
	// ========================================================================
	else if (currentStage == 6) {
		rewards = {
			{ new AirReward(), 20.0f },
			{ new StrongTouchReward(30, 200), 350 },
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 150.0f },
			{ new VelocityPlayerToBallReward(), 10.f },
			{ new FaceBallReward(), 0.8f },
			{ new PickupBoostReward(), 15.f },
			{ new SaveBoostReward(), 4.0f },
			{ new GoalReward(), 800 }
		};
		terminalConditions = {
			new NoTouchCondition(10),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
// STAGE 7: PRO 2V2 GAME SENSE (ANTI-BALLCHASING)
// ========================================================================
else if (currentStage == 7) {
	rewards = {
		// Ball play (moderate)
		{ new AirReward(), 8.0f },
		{ new StrongTouchReward(25, 180), 120 },  // Reduced from 200
		
		// Goal direction (ZeroSum OK for competitive)
		{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 100.0f },
		
		// Movement (HEAVILY REDUCED)
		{ new VelocityPlayerToBallReward(), 2.f },  // Was 6, now 2!
		{ new FaceBallReward(), 0.3f },             // Was 1.0, now 0.3!
		
		// Boost management
		{ new PickupBoostReward(), 12.f },
		{ new SaveBoostReward(), 5.0f },            // Increased from 3
		
		// Competitive
		{ new ZeroSumReward(new BumpReward(), 0.5f), 40 },
		{ new ZeroSumReward(new DemoReward(), 0.5f), 120 },
		
		// GOALS ARE EVERYTHING
		{ new GoalReward(), 800 }  // Increased from 500
	};
	terminalConditions = {
		new NoTouchCondition(10),
		new GoalScoreCondition()
	};
}

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
	result.stateSetter = new KickoffState();  // Always use kickoffs
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
	
	report.AddAvg("Training/Current Stage", currentStage);
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char* argv[]) {
	// Set Python path for metrics (Windows)
#ifdef _WIN32
	_putenv("PYTHONPATH=C:\\Users\\Jake\\Videos\\Jake\\GigaLearnCPP-Leak-main");
#endif

	RocketSim::Init("C:\\Users\\Jake\\Videos\\Jake\\GigaLearnCPP-Leak-main\\collision_meshes");

	std::cout << "========================================" << std::endl;
	std::cout << "GigaLearnCPP - 7-Stage Curriculum" << std::endl;
	std::cout << "2v2 Pro Bot with Speed Flip Kickoffs" << std::endl;
	std::cout << "========================================" << std::endl;

	struct StageConfig {
		int stageNum;
		std::string name;
		int timesteps;
		float policyLR;
		float criticLR;
	};

	std::vector<StageConfig> stages = {
		//{1, "Ball Contact", 100'000'000, 3e-4f, 3e-4f},
  // {2, "Goal Shooting", 200'000'000, 3e-4f, 3e-4f},
    {3, "Power & Accuracy", 300'000'000, 2e-4f, 2e-4f},
   // {4, "Aerial Fundamentals", 500'000'000, 2e-4f, 2e-4f},
   // {5, "Air Dribbles", 600'000'000, 1.5e-4f, 1.5e-4f},
   // {6, "Double Taps", 600'000'000, 1.5e-4f, 1.5e-4f},
	//	{7, "Pro 2v2 Game Sense", 1'000'000'000, 1e-4f, 1e-4f}
	};

	for (auto& stageConfig : stages) {
		currentStage = stageConfig.stageNum;
		
		std::cout << "\n========================================" << std::endl;
		std::cout << "STAGE " << currentStage << "/7: " << stageConfig.name << std::endl;
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
		cfg.ppo.policyLR = stageConfig.policyLR;
		cfg.ppo.criticLR = stageConfig.criticLR;

		if (currentStage <= 3) {
			cfg.ppo.sharedHead.layerSizes = { 256, 256 };
			cfg.ppo.policy.layerSizes = { 256, 256, 256 };
			cfg.ppo.critic.layerSizes = { 256, 256, 256 };
		} else {
			cfg.ppo.sharedHead.layerSizes = { 512, 512 };
			cfg.ppo.policy.layerSizes = { 512, 512, 256 };
			cfg.ppo.critic.layerSizes = { 512, 512, 256 };
		}

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

		// Metrics and rendering - JUST CHANGE THESE!
		cfg.sendMetrics = true;   // Set true for metrics
		cfg.renderMode = false;    // Set true for RocketSimVis

		Learner* learner = new Learner(EnvCreateFunc, cfg, StepCallback);
		
		std::cout << "Starting Stage " << currentStage << " training..." << std::endl;
		learner->Start();

		delete learner;
		std::cout << "✓ Stage " << currentStage << " complete!\n" << std::endl;
	}

	std::cout << "\n🎉 ALL 7 STAGES COMPLETE!" << std::endl;
	return EXIT_SUCCESS;
}
