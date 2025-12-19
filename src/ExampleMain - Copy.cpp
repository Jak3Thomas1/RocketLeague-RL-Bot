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
// ENV CREATION - 7 STAGE CURRICULUM
// ===============================
EnvCreateResult EnvCreateFunc(int index) {
    std::vector<WeightedReward> rewards;
    std::vector<TerminalCondition*> terminalConditions;

    switch (currentStage) {
        case 1: // Ball Contact - DISCOURAGE FLIPS
            rewards = {
                { new StrongTouchReward(5,50), 100 },
                { new FaceBallReward(), 5 },
                { new VelocityPlayerToBallReward(), 10 },
                { new PickupBoostReward(), 5 },
                { new GoalReward(), 200 },
                { new AirReward(), 0.01f }  // VERY LOW - punish being in air
            };
            terminalConditions = { new NoTouchCondition(15), new GoalScoreCondition() };
            break;
        case 2: // Goal Shooting - STILL DISCOURAGE FLIPS
            rewards = {
                { new StrongTouchReward(5,50), 15 },
                { new ZeroSumReward(new VelocityBallToGoalReward(), 1), 80 },
                { new VelocityPlayerToBallReward(), 8 },
                { new FaceBallReward(), 4 },
                { new PickupBoostReward(), 8 },
                { new SaveBoostReward(), 1 },
                { new GoalReward(), 400 },
                { new AirReward(), 0.01f }  // VERY LOW - keep discouraging flips
            };
            terminalConditions = { new NoTouchCondition(12), new GoalScoreCondition() };
            break;
        case 3: // Power & Accuracy - FLIPS UNLOCKED!
            rewards = {
                { new StrongTouchReward(20,150), 150 },
                { new ZeroSumReward(new VelocityBallToGoalReward(),1), 80 },
                { new VelocityPlayerToBallReward(), 6 },
                { new FaceBallReward(), 1.5f },
                { new PickupBoostReward(), 10 },
                { new SaveBoostReward(), 2 },
                { new ZeroSumReward(new BumpReward(),0.5f), 30 },
                { new GoalReward(), 400 },
                { new AirReward(), 0.2f }  // HIGHER - allow flips now
            };
            terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
            break;
        case 4: // Aerial Fundamentals
            rewards = {
                { new AirReward(), 0.25f },
                { new StrongTouchReward(20,150), 200 },
                { new ZeroSumReward(new VelocityBallToGoalReward(),1), 100 },
                { new VelocityPlayerToBallReward(), 5 },
                { new FaceBallReward(), 1 },
                { new PickupBoostReward(), 12 },
                { new SaveBoostReward(), 3 },
                { new GoalReward(), 500 }
            };
            terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
            break;
        case 5: // Air Dribbles
            rewards = {
                { new AirReward(), 25 },
                { new StrongTouchReward(30,200), 300 },
                { new ZeroSumReward(new VelocityBallToGoalReward(),1), 120 },
                { new VelocityPlayerToBallReward(), 8 },
                { new PickupBoostReward(), 15 },
                { new SaveBoostReward(), 4 },
                { new GoalReward(), 600 }
            };
            terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
            break;
        case 6: // Double Taps & Wall Play
            rewards = {
                { new AirReward(), 20 },
                { new StrongTouchReward(30,200), 350 },
                { new ZeroSumReward(new VelocityBallToGoalReward(),1), 150 },
                { new VelocityPlayerToBallReward(), 10 },
                { new FaceBallReward(), 0.8f },
                { new PickupBoostReward(), 15 },
                { new SaveBoostReward(), 4 },
                { new GoalReward(), 800 }
            };
            terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
            break;
        case 7: // Pro 2v2 Game Sense
            rewards = {
                { new AirReward(), 8 },
                { new StrongTouchReward(25,180), 120 },
                { new ZeroSumReward(new VelocityBallToGoalReward(),1), 100 },
                { new VelocityPlayerToBallReward(), 2 },
                { new FaceBallReward(), 0.3f },
                { new PickupBoostReward(), 12 },
                { new SaveBoostReward(), 5 },
                { new ZeroSumReward(new BumpReward(),0.5f), 40 },
                { new ZeroSumReward(new DemoReward(),0.5f), 120 },
                { new GoalReward(), 800 }
            };
            terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
            break;
    }

    int playersPerTeam = 2;
    auto arena = Arena::Create(GameMode::SOCCAR);
    for (int i = 0; i < playersPerTeam; i++) {
        arena->AddCar(Team::BLUE);
        arena->AddCar(Team::ORANGE);
    }

    EnvCreateResult result = {};
    result.actionParser = new DefaultAction();  // NO CUSTOM PARSER
    result.obsBuilder = new AdvancedObs();
    result.stateSetter = new KickoffState();
    result.terminalConditions = terminalConditions;
    result.rewards = rewards;
    result.arena = arena;
    return result;
}

// ===============================
// STEP CALLBACK (metrics & auto-stage)
// ===============================
void StepCallback(Learner* learner, const std::vector<GameState>& states, Report& report) {
    static int timestepsInStage = 0;
    static int goalsScored = 0;
    static int stepsProcessed = 0;
    
    for (auto& state : states) {
        if (state.goalScored) goalsScored++;
        stepsProcessed++;
    }
    
    timestepsInStage += states.size();

    // Auto stage advancement
    if (timestepsInStage >= 100000000) {  // 100M steps minimum per stage
        float goalRate = (float)goalsScored / (float)stepsProcessed;
        
        if (goalRate > 0.15f && currentStage < 7) {  // 15% goal rate to advance
            currentStage++;
            std::cout << "\n========================================" << std::endl;
            std::cout << "✅ ADVANCED TO STAGE " << currentStage << std::endl;
            if (currentStage == 3) {
                std::cout << "🔓 FLIPS NOW REWARDED (AirReward increased)!" << std::endl;
            }
            std::cout << "========================================\n" << std::endl;
            timestepsInStage = 0;
            goalsScored = 0;
            stepsProcessed = 0;
        }
    }

    report.AddAvg("Training/Current Stage", (float)currentStage);
    report.AddAvg("Training/Timesteps In Stage", (float)timestepsInStage);
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
    std::cout << "🎮 7-STAGE - REWARD-BASED FLIP CONTROL" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "FLIP CONTROL VIA REWARDS:" << std::endl;
    std::cout << "  Stage 1-2: AirReward=0.01 (punish flips)" << std::endl;
    std::cout << "  Stage 3+:  AirReward=0.2+ (reward flips)" << std::endl;
    std::cout << "\nPerformance:" << std::endl;
    std::cout << "  • 1024 GAMES" << std::endl;
    std::cout << "  • 512 network" << std::endl;
    std::cout << "  • Expected SPS: 100k+" << std::endl;
    std::cout << "\nStages:" << std::endl;
    std::cout << "  1. Ball Contact (0-100M)" << std::endl;
    std::cout << "  2. Goal Shooting (100-200M)" << std::endl;
    std::cout << "  3. Power & Accuracy (200-300M) ← FLIPS REWARDED" << std::endl;
    std::cout << "  4. Aerial Fundamentals (300-400M)" << std::endl;
    std::cout << "  5. Air Dribbles (400-500M)" << std::endl;
    std::cout << "  6. Double Taps (500-600M)" << std::endl;
    std::cout << "  7. Pro 2v2 (600M+)" << std::endl;
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
        std::cout << "RENDER MODE\n" << std::endl;
    } else {
        cfg.sendMetrics = true;
        cfg.renderMode = true;
    }

    std::cout << "Starting Stage " << currentStage << "...\n" << std::endl;

    Learner* learner = new Learner(EnvCreateFunc, cfg, StepCallback);
    
    // FORCE STAGE OVERRIDE (happens after checkpoint load!)
    currentStage = 3;  // Change this number to force a stage
    std::cout << "\n🔧 MANUALLY FORCED TO STAGE " << currentStage << "!\n" << std::endl;
    
    learner->Start();
    delete learner;

    std::cout << "\n🎉 TRAINING COMPLETE!" << std::endl;
    return EXIT_SUCCESS;
}