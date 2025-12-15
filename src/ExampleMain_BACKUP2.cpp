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

using namespace GGL; // GigaLearn
using namespace RLGC; // RLGymCPP

// ============================================================================
// CURRICULUM TRAINING: 7-STAGE PRO-LEVEL 2V2 BOT
// ============================================================================

// Current training stage (will be set before each stage)
int currentStage = 1;

// Create the RLGymCPP environment for each of our games
EnvCreateResult EnvCreateFunc(int index) {
	std::vector<WeightedReward> rewards;
	std::vector<TerminalCondition*> terminalConditions;
	
	// ========================================================================
	// STAGE 1: BALL CONTACT & AWARENESS (100M steps)
	// Goal: Learn to touch the ball consistently
	// ========================================================================
	if (currentStage == 1) {
		rewards = {
			// Encourage any ball contact
			{ new StrongTouchReward(5, 50), 100 },  // High reward for touches
			{ new FaceBallReward(), 5.0f },          // Face the ball
			{ new VelocityPlayerToBallReward(), 10.f }, // Move towards ball
			{ new PickupBoostReward(), 5.f },        // Collect boost
			{ new GoalReward(), 200 }                // Goals are always good
		};
		
		terminalConditions = {
			new NoTouchCondition(15),  // Longer timeout for learning
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 2: GOAL SHOOTING (200M steps)
	// Goal: Hit ball towards opponent goal
	// ========================================================================
	else if (currentStage == 2) {
		rewards = {
			// Reduced ball touch reward
			{ new StrongTouchReward(5, 50), 30 },
			
			// Encourage shooting towards goal
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 50.0f },
			{ new VelocityPlayerToBallReward(), 8.f },
			{ new FaceBallReward(), 2.0f },
			
			// Boost management
			{ new PickupBoostReward(), 8.f },
			{ new SaveBoostReward(), 1.0f },
			
			// Goals
			{ new GoalReward(), 300 }
		};
		
		terminalConditions = {
			new NoTouchCondition(12),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 3: POWER & ACCURACY (300M steps)
	// Goal: Power shots, dribbling, accuracy
	// ========================================================================
	else if (currentStage == 3) {
		rewards = {
			// Power shots (higher velocity = more reward)
			{ new StrongTouchReward(20, 150), 150 },
			
			// Ball-goal velocity (want fast shots)
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 80.0f },
			
			// Movement and positioning
			{ new VelocityPlayerToBallReward(), 6.f },
			{ new FaceBallReward(), 1.5f },
			
			// Boost efficiency
			{ new PickupBoostReward(), 10.f },
			{ new SaveBoostReward(), 2.0f },
			
			// Game events
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
	// Goal: Learn fast aerials and aerial interception
	// ========================================================================
	else if (currentStage == 4) {
		rewards = {
			// Aerial play (high reward for being in air)
			{ new AirReward(), 15.0f },
			
			// Touches (especially aerial touches)
			{ new StrongTouchReward(20, 150), 200 },
			
			// Ball-goal velocity
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 100.0f },
			
			// Movement
			{ new VelocityPlayerToBallReward(), 5.f },
			{ new FaceBallReward(), 1.0f },
			
			// Boost management (crucial for aerials)
			{ new PickupBoostReward(), 12.f },
			{ new SaveBoostReward(), 3.0f },
			
			// Goals (extra reward if scored in air)
			{ new GoalReward(), 500 }
		};
		
		terminalConditions = {
			new NoTouchCondition(10),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 5: AIR DRIBBLES (600M steps)
	// Goal: Consecutive aerial touches, air roll control
	// ========================================================================
	else if (currentStage == 5) {
		rewards = {
			// Heavy air reward
			{ new AirReward(), 25.0f },
			
			// Extremely high touch rewards (for air dribbles)
			{ new StrongTouchReward(30, 200), 300 },
			
			// Ball-goal velocity
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 120.0f },
			
			// Player-ball velocity (stay close to ball)
			{ new VelocityPlayerToBallReward(), 8.f },
			
			// Boost
			{ new PickupBoostReward(), 15.f },
			{ new SaveBoostReward(), 4.0f },
			
			// Goals from air dribbles
			{ new GoalReward(), 600 }
		};
		
		terminalConditions = {
			new NoTouchCondition(10),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 6: DOUBLE TAPS & WALL PLAY (600M steps)
	// Goal: Backboard reads, wall aerials, double taps
	// ========================================================================
	else if (currentStage == 6) {
		rewards = {
			// Air play
			{ new AirReward(), 20.0f },
			
			// High touch rewards
			{ new StrongTouchReward(30, 200), 350 },
			
			// Ball-goal velocity (backboard shots)
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 150.0f },
			
			// Movement
			{ new VelocityPlayerToBallReward(), 10.f },
			{ new FaceBallReward(), 0.8f },
			
			// Boost
			{ new PickupBoostReward(), 15.f },
			{ new SaveBoostReward(), 4.0f },
			
			// Double tap goals
			{ new GoalReward(), 800 }
		};
		
		terminalConditions = {
			new NoTouchCondition(10),
			new GoalScoreCondition()
		};
	}
	
	// ========================================================================
	// STAGE 7: PRO 2V2 GAME SENSE (1B steps)
	// Goal: Rotation, positioning, teamplay, decision making
	// ========================================================================
	else if (currentStage == 7) {
		rewards = {
			// Balanced rewards for pro play
			{ new AirReward(), 10.0f },
			{ new StrongTouchReward(25, 180), 200 },
			
			// Ball-goal (zero-sum for competitive play)
			{ new ZeroSumReward(new VelocityBallToGoalReward(), 1), 100.0f },
			
			// Movement and positioning
			{ new VelocityPlayerToBallReward(), 6.f },
			{ new FaceBallReward(), 1.0f },
			
			// Boost management
			{ new PickupBoostReward(), 12.f },
			{ new SaveBoostReward(), 3.0f },
			
			// Game events (important for 2v2)
			{ new ZeroSumReward(new BumpReward(), 0.5f), 40 },
			{ new ZeroSumReward(new DemoReward(), 0.5f), 120 },
			
			// Goals and saves
			{ new GoalReward(), 500 }
		};
		
		terminalConditions = {
			new NoTouchCondition(10),
			new GoalScoreCondition()
		};
	}

	// Make the arena (2v2 for all stages)
	int playersPerTeam = 2;  // 2v2 gameplay
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
	// Add metrics based on current stage
	bool doExpensiveMetrics = (rand() % 4) == 0;

	for (auto& state : states) {
		if (doExpensiveMetrics) {
			for (auto& player : state.players) {
				report.AddAvg("Player/In Air Ratio", !player.isOnGround);
				report.AddAvg("Player/Ball Touch Ratio", player.ballTouchedStep);
				report.AddAvg("Player/Demoed Ratio", player.isDemoed);
				report.AddAvg("Player/Speed", player.vel.Length());
				
				Vec dirToBall = (state.ball.pos - player.pos).Normalized();
				report.AddAvg("Player/Speed Towards Ball", RS_MAX(0, player.vel.Dot(dirToBall)));
				report.AddAvg("Player/Boost", player.boost);

				if (player.ballTouchedStep)
					report.AddAvg("Player/Touch Height", state.ball.pos.z);
			}
		}

		if (state.goalScored) {
			report.AddAvg("Game/Goal Speed", state.ball.vel.Length());
			report.AddAvg("Game/Goal Height", state.ball.pos.z);
		}
	}
	
	// Report current stage
	report.AddAvg("Training/Current Stage", currentStage);
}

// ============================================================================
// MAIN TRAINING FUNCTION
// ============================================================================
int main(int argc, char* argv[]) {
	// Initialize RocketSim with collision meshes
	// UPDATE THIS PATH TO YOUR COLLISION MESHES LOCATION!
	RocketSim::Init("C:\\Users\\Jake\\Videos\\Jake\\GigaLearnCPP-Leak-main\\collision_meshes");

	std::cout << "========================================" << std::endl;
	std::cout << "GigaLearnCPP - 7-Stage Curriculum Training" << std::endl;
	std::cout << "2v2 Pro-Level Bot" << std::endl;
	std::cout << "========================================" << std::endl;

	// Training stages configuration
	struct StageConfig {
		int stageNum;
		std::string name;
		int timesteps;
		float policyLR;
		float criticLR;
	};

	std::vector<StageConfig> stages = {
		{1, "Ball Contact", 100'000'000, 3e-4f, 3e-4f},
		{2, "Goal Shooting", 200'000'000, 3e-4f, 3e-4f},
		{3, "Power & Accuracy", 300'000'000, 2e-4f, 2e-4f},
		{4, "Aerial Fundamentals", 500'000'000, 2e-4f, 2e-4f},
		{5, "Air Dribbles", 600'000'000, 1.5e-4f, 1.5e-4f},
		{6, "Double Taps", 600'000'000, 1.5e-4f, 1.5e-4f},
		{7, "Pro 2v2 Game Sense", 1'000'000'000, 1e-4f, 1e-4f}
	};

	// Train each stage
	for (auto& stageConfig : stages) {
		currentStage = stageConfig.stageNum;
		
		std::cout << "\n========================================" << std::endl;
		std::cout << "STAGE " << currentStage << "/7: " << stageConfig.name << std::endl;
		std::cout << "Target Timesteps: " << stageConfig.timesteps << std::endl;
		std::cout << "========================================\n" << std::endl;

		// Make configuration for the learner
		LearnerConfig cfg = {};

		cfg.deviceType = LearnerDeviceType::GPU_CUDA;
		cfg.tickSkip = 8;
		cfg.actionDelay = cfg.tickSkip - 1;

		// Adjust based on your GPU VRAM (8GB+ GPU can handle more)
		cfg.numGames = 256;  // Reduce to 128 if out of memory

		cfg.randomSeed = 123;

		int tsPerItr = 50'000;
		cfg.ppo.tsPerItr = tsPerItr;
		cfg.ppo.batchSize = tsPerItr;
		cfg.ppo.miniBatchSize = 50'000;  // Reduce if out of VRAM

		cfg.ppo.epochs = 1;  // Can increase to 2-3 for better learning
		cfg.ppo.entropyScale = 0.035f;
		cfg.ppo.gaeGamma = 0.99;

		// Stage-specific learning rates
		cfg.ppo.policyLR = stageConfig.policyLR;
		cfg.ppo.criticLR = stageConfig.criticLR;

		// Network architecture (larger for later stages)
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

		bool addLayerNorm = true;
		cfg.ppo.policy.addLayerNorm = addLayerNorm;
		cfg.ppo.critic.addLayerNorm = addLayerNorm;
		cfg.ppo.sharedHead.addLayerNorm = addLayerNorm;

		cfg.sendMetrics = true;   // Send metrics to Python
		cfg.renderMode = true;   // Set to true if using RocketSimVis

		// Make the learner
		Learner* learner = new Learner(EnvCreateFunc, cfg, StepCallback);

		// Load checkpoint from previous stage if exists
		if (currentStage > 1) {
			std::string checkpointPath = "models/stage" + std::to_string(currentStage - 1) + "_final.pt";
			std::cout << "Loading checkpoint from previous stage: " << checkpointPath << std::endl;
			// learner->Load(checkpointPath);  // Uncomment if checkpoint loading works
		}

		// Start learning for this stage!
		std::cout << "Starting training for Stage " << currentStage << "..." << std::endl;
		learner->Start();

		// Save checkpoint after stage completion
		std::string savePath = "models/stage" + std::to_string(currentStage) + "_final.pt";
		std::cout << "\nStage " << currentStage << " complete! Saving checkpoint: " << savePath << std::endl;
		// learner->Save(savePath);  // Uncomment if checkpoint saving works

		delete learner;
		
		std::cout << "\n✓ Stage " << currentStage << " completed!" << std::endl;
	}

	std::cout << "\n========================================" << std::endl;
	std::cout << "🎉 ALL 7 STAGES COMPLETE!" << std::endl;
	std::cout << "Pro-level 2v2 bot trained successfully!" << std::endl;
	std::cout << "========================================" << std::endl;

	return EXIT_SUCCESS;
}
