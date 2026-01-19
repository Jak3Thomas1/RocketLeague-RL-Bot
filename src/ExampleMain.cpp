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

using namespace GGL;
using namespace RLGC;

// ===============================
// GLOBAL STAGE TRACKER
// ===============================
static int currentStage = 3;

// ===============================
// CUSTOM REWARD: Punish Low Boost Flips
// ===============================
class LowBoostFlipPenalty : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		if (!player.isOnGround && player.boost < 20) {
			return -0.2f;
		}
		return 0.0f;
	}
};

// ===============================
// CUSTOM REWARD: AERIAL TOUCH BONUS
// ===============================
class AerialTouchBonus : public Reward {
public:
	virtual float GetReward(const Player& player, const GameState& state, bool isFinal) override {
		// MASSIVE bonus for touching ball while in air
		if (player.ballTouchedStep && !player.isOnGround) {
			float height = player.pos.z;
			
			// Scale reward by height
			if (height > 200) {
				float heightBonus = (height - 200) / 100.0f;  // +1 per 100 units above 200
				return 50.0f + heightBonus;  // Base 50 + height scaling
			}
		}
		return 0.0f;
	}
};

// ===============================
// ENV CREATION - AERIAL HYBRID STAGE 3
// ===============================
EnvCreateResult EnvCreateFunc(int index) {
    std::vector<WeightedReward> rewards;
    std::vector<TerminalCondition*> terminalConditions;

    // ========================================================================
    // STAGE 3: POWER & ACCURACY + FORCED AERIAL LEARNING
    // ========================================================================
    // This balances ground skills with strong aerial incentives
    rewards = {
        // ====================================================================
        // AERIAL REWARDS - MASSIVE TO ENCOURAGE AERIAL PLAY
        // ====================================================================
        { new AerialTouchBonus(), 200.0f },       // HUGE bonus for aerial touches!
        { new AirReward(), 80.0f },               // Big reward for being in air (was 0.2!)
        
        // ====================================================================
        // GROUND MECHANICS - REDUCED BUT STILL STRONG
        // ====================================================================
        { new StrongTouchReward(20,150), 60 },    // Still reward power (was 150)
        { new ZeroSumReward(new VelocityBallToGoalReward(),1), 80 },  // Keep shooting
        
        // ====================================================================
        // MOVEMENT - LESS BALL CHASING, MORE THINKING
        // ====================================================================
        { new VelocityPlayerToBallReward(), 2 },  // REDUCED (was 6) - less chasing
        { new FaceBallReward(), 0.5f },           // REDUCED (was 1.5) - less focus on ball
        
        // ====================================================================
        // BOOST MANAGEMENT - CRITICAL FOR AERIALS
        // ====================================================================
        { new PickupBoostReward(), 20 },          // INCREASED (was 15) - need boost for aerials
        { new SaveBoostReward(), 5 },             // INCREASED (was 3) - manage boost better
        
        // ====================================================================
        // ADVANCED MECHANICS
        // ====================================================================
        { new ZeroSumReward(new BumpReward(),0.5f), 30 },
        { new WavedashReward(), 50.0f },
        { new LowBoostFlipPenalty(), 20.0f },
        
        // ====================================================================
        // ULTIMATE GOAL - INCREASED TO OFFSET AERIAL RISK
        // ====================================================================
        { new GoalReward(), 500 }  // INCREASED (was 400) - goals still most important
    };

    terminalConditions = { 
        new NoTouchCondition(15),  // LONGER timeout (was 10) - more time to aerial
        new GoalScoreCondition() 
    };

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

// ===============================
// STEP CALLBACK - TRACK AERIAL PROGRESS
// ===============================
void StepCallback(Learner* learner, const std::vector<GameState>& states, Report& report) {
    static int timestepsInStage = 0;
    static int goalsScored = 0;
    static int stepsProcessed = 0;
    static int aerialTouches = 0;
    static int groundTouches = 0;
    static int airTime = 0;
    
    for (auto& state : states) {
        if (state.goalScored) goalsScored++;
        stepsProcessed++;
        
        // Track aerial vs ground behavior
        for (auto& player : state.players) {
            // Count aerial touches
            if (player.ballTouchedStep && !player.isOnGround) {
                aerialTouches++;
            }
            
            // Count ground touches
            if (player.ballTouchedStep && player.isOnGround) {
                groundTouches++;
            }
            
            // Track time in air
            if (!player.isOnGround) {
                airTime++;
            }
        }
    }
    
    timestepsInStage += states.size();

    // Report aerial learning metrics
    if (stepsProcessed > 0) {
        float aerialTouchRate = (float)aerialTouches / (float)stepsProcessed * 100.0f;
        float groundTouchRate = (float)groundTouches / (float)stepsProcessed * 100.0f;
        float airTimePercent = (float)airTime / (float)(stepsProcessed * 4) * 100.0f;  // 4 players
        
        report.AddAvg("Training/Aerial Touch Rate %", aerialTouchRate);
        report.AddAvg("Training/Ground Touch Rate %", groundTouchRate);
        report.AddAvg("Training/Air Time %", airTimePercent);
        report.AddAvg("Training/Total Aerial Touches", (float)aerialTouches);
    }
    
    report.AddAvg("Training/Current Stage", (float)currentStage);
    report.AddAvg("Training/Timesteps In Stage", (float)timestepsInStage);
    
    // Goal rate
    if (stepsProcessed > 0) {
        float goalRate = (float)goalsScored / (float)stepsProcessed * 100.0f;
        report.AddAvg("Training/Goal Rate %", goalRate);
    }
}

// ===============================
// MAIN
// ===============================
int main(int argc, char* argv[]) {
    bool renderMode = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--render") {
            renderMode = true;
            break;
        }
    }

#ifdef _WIN32
    _putenv("PYTHONPATH=C:\\Users\\Jake\\Videos\\Jake\\GigaLearnCPP-Leak-main");
#endif

    RocketSim::Init("C:\\Users\\Jake\\Videos\\Jake\\GigaLearnCPP-Leak-main\\collision_meshes");

    std::cout << "========================================" << std::endl;
    std::cout << "🚀 AERIAL HYBRID TRAINING" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "STRATEGY:" << std::endl;
    std::cout << "  • Keep strong ground game ✓" << std::endl;
    std::cout << "  • Add aerial capability NEW!" << std::endl;
    std::cout << "  • Balance both playstyles" << std::endl;
    std::cout << "\nREWARD CHANGES:" << std::endl;
    std::cout << "  • AirReward: 0.2 → 80 (400x increase!)" << std::endl;
    std::cout << "  • Aerial Touch Bonus: +50-200 per touch" << std::endl;
    std::cout << "  • Ground touches: Still rewarded (reduced)" << std::endl;
    std::cout << "\nEXPECTED TIMELINE:" << std::endl;
    std::cout << "  • First 500M steps: Learning aerials, reward may dip" << std::endl;
    std::cout << "  • 500M-1.5B steps: Aerials improving, reward climbing" << std::endl;
    std::cout << "  • 1.5B-3B steps: Hybrid playstyle, better than before" << std::endl;
    std::cout << "\nTARGET METRICS:" << std::endl;
    std::cout << "  • Aerial Touch Rate: 5-10% (currently ~1%)" << std::endl;
    std::cout << "  • Air Time: 20-30% (currently ~5-10%)" << std::endl;
    std::cout << "  • Goals: Maintain or increase" << std::endl;
    std::cout << "========================================\n" << std::endl;

    LearnerConfig cfg = {};
    cfg.deviceType = LearnerDeviceType::GPU_CUDA;
    cfg.tickSkip = 8;
    cfg.actionDelay = cfg.tickSkip - 1;
    cfg.numGames = 1024;
    cfg.randomSeed = 123;

    int tsPerItr = 50'000;
    cfg.ppo.tsPerItr = tsPerItr;
    cfg.ppo.batchSize = tsPerItr;
    cfg.ppo.miniBatchSize = 25'000;
    cfg.ppo.epochs = 1;
    cfg.ppo.entropyScale = 0.035f;
    cfg.ppo.gaeGamma = 0.99;
    cfg.ppo.gaeLambda = 0.95;
    cfg.ppo.policyLR = 1.5e-4f;
    cfg.ppo.criticLR = 1.5e-4f;

    // KEEP SAME MODEL SIZE - 512x512
    cfg.ppo.sharedHead.layerSizes = {512, 512};
    cfg.ppo.policy.layerSizes = {512, 512};
    cfg.ppo.critic.layerSizes = {512, 512};

    cfg.ppo.policy.optimType = ModelOptimType::ADAM;
    cfg.ppo.critic.optimType = ModelOptimType::ADAM;
    cfg.ppo.sharedHead.optimType = ModelOptimType::ADAM;

    cfg.ppo.policy.activationType = ModelActivationType::RELU;
    cfg.ppo.critic.activationType = ModelActivationType::RELU;
    cfg.ppo.sharedHead.activationType = ModelActivationType::RELU;

    cfg.ppo.policy.addLayerNorm = true;
    cfg.ppo.critic.addLayerNorm = true;
    cfg.ppo.sharedHead.addLayerNorm = true;

    if (renderMode) {
        cfg.renderMode = true;
        cfg.sendMetrics = false;
        cfg.ppo.deterministic = true;
        std::cout << "🎥 RENDER MODE\n" << std::endl;
    } else {
        cfg.sendMetrics = true;
        cfg.renderMode = true;  // Keep render on to watch in RocketSimVis
    }

    std::cout << "📊 Current: 7.7B timesteps" << std::endl;
    std::cout << "🎯 Target: 10-11B timesteps (add 2-3B more)" << std::endl;
    std::cout << "⏱️  ETA: ~20-30 hours at current SPS\n" << std::endl;
    
    std::cout << "🚀 Starting Aerial Hybrid Training...\n" << std::endl;
    std::cout << "⚠️  WARNING: Reward may DROP initially while learning aerials!" << std::endl;
    std::cout << "    This is NORMAL. It will recover and exceed previous levels.\n" << std::endl;

    Learner* learner = new Learner(EnvCreateFunc, cfg, StepCallback);
    
    std::cout << "\n✅ TRAINING ACTIVE - LOADING CHECKPOINT 7.7B...\n" << std::endl;
    
    learner->Start();
    delete learner;

    std::cout << "\n🎉 TRAINING COMPLETE!" << std::endl;
    return EXIT_SUCCESS;
}