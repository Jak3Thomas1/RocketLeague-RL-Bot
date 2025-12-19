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

int currentStage = 7;
int kickoffTick = 0;
bool isKickoff = false;

// ============================================================================
// HARDCODED KICKOFF SEQUENCE
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
    if (tick < 44) {
        action.throttle = 1.0f;
        action.boost = true;
        if (tick >= 11 * 4) { action.steer = -0.3f; }
    } else if (tick < 52) {
        action.throttle = 1.0f;
        action.jump = true;
        action.boost = true;
    } else if (tick < 56) {
        action.throttle = 1.0f;
        action.boost = true;
    } else if (tick < 60) {
        action.throttle = 1.0f;
        action.yaw = 0.8f;
        action.pitch = -0.7f;
        action.jump = true;
        action.boost = true;
    } else if (tick < 112) {
        action.throttle = 1.0f;
        action.pitch = 1.0f;
        action.boost = true;
    } else if (tick < 152) {
        action.throttle = 1.0f;
        action.roll = 1.0f;
        action.pitch = 0.5f;
    } else {
        action.throttle = 0;
    }
    return action;
}

// ============================================================================
// NO-FLIP ACTION PARSER FOR STAGE 2
// ============================================================================
struct NoFlipAction : DefaultAction {
    Action ParseAction(int playerIndex, const Player& player, const GameState& state) override {
        Action act = DefaultAction::ParseAction(playerIndex, player, state);
        act.jump = false;
        act.pitch = 0;
        act.yaw = 0;
        act.roll = 0;
        return act;
    }
};

// ============================================================================
// ENVIRONMENT CREATION
// ============================================================================
EnvCreateResult EnvCreateFunc(int index) {
    std::vector<WeightedReward> rewards;
    std::vector<TerminalCondition*> terminalConditions;

    if (currentStage == 1) {
        rewards = {
            { new StrongTouchReward(5, 50), 100 },
            { new FaceBallReward(), 5.0f },
            { new VelocityPlayerToBallReward(), 10.f },
            { new PickupBoostReward(), 5.f },
            { new GoalReward(), 200 }
        };
        terminalConditions = { new NoTouchCondition(15), new GoalScoreCondition() };
    } else if (currentStage == 2) {
        rewards = {
            { new StrongTouchReward(5, 50), 15 },
            { new VelocityBallToGoalReward(), 80.0f },
            { new VelocityPlayerToBallReward(), 8.f },
            { new FaceBallReward(), 4.0f },
            { new PickupBoostReward(), 8.f },
            { new SaveBoostReward(), 1.0f },
            { new GoalReward(), 400 },
        };
        terminalConditions = {
            new NoTouchCondition(12),
            new GoalScoreCondition()
        };
    } else if (currentStage == 3) {
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
        terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
    } else if (currentStage == 4) {
        rewards = {
            { new AirReward(), 0.03f },
            { new StrongTouchReward(20, 150), 200 },
            { new ZeroSumReward(new VelocityBallToGoalReward(), 1), 100.0f },
            { new VelocityPlayerToBallReward(), 5.f },
            { new FaceBallReward(), 1.0f },
            { new PickupBoostReward(), 12.f },
            { new SaveBoostReward(), 3.0f },
            { new GoalReward(), 500 }
        };
        terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
    } else if (currentStage == 5) {
        rewards = {
            { new AirReward(), 25.0f },
            { new StrongTouchReward(30, 200), 300 },
            { new ZeroSumReward(new VelocityBallToGoalReward(), 1), 120.0f },
            { new VelocityPlayerToBallReward(), 8.f },
            { new PickupBoostReward(), 15.f },
            { new SaveBoostReward(), 4.0f },
            { new GoalReward(), 600 }
        };
        terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
    } else if (currentStage == 6) {
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
        terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
    } else if (currentStage == 7) {
        rewards = {
            { new AirReward(), 8.0f },
            { new StrongTouchReward(25, 180), 120 },
            { new ZeroSumReward(new VelocityBallToGoalReward(), 1), 100.0f },
            { new VelocityPlayerToBallReward(), 2.f },
            { new FaceBallReward(), 0.3f },
            { new PickupBoostReward(), 12.f },
            { new SaveBoostReward(), 5.0f },
            { new ZeroSumReward(new BumpReward(), 0.5f), 40 },
            { new ZeroSumReward(new DemoReward(), 0.5f), 120 },
            { new GoalReward(), 800 }
        };
        terminalConditions = { new NoTouchCondition(10), new GoalScoreCondition() };
    }

    int playersPerTeam = 2;
    auto arena = Arena::Create(GameMode::SOCCAR);
    for (int i = 0; i < playersPerTeam; i++) {
        arena->AddCar(Team::BLUE);
        arena->AddCar(Team::ORANGE);
    }

    EnvCreateResult result = {};
    result.actionParser = (currentStage == 2) ? static_cast<ActionParser*>(new NoFlipAction()) : new DefaultAction();
    result.obsBuilder = new AdvancedObs();
    result.stateSetter = new KickoffState();
    result.terminalConditions = terminalConditions;
    result.rewards = rewards;
    result.arena = arena;
    return result;
}

// ============================================================================
// STEP CALLBACK
// ============================================================================
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
        {2, "Goal Shooting", 200'000'000, 3e-4f, 3e-4f}
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

        Learner* learner = new Learner(EnvCreateFunc, cfg, StepCallback);
        std::cout << "Starting Stage " << currentStage << " training..." << std::endl;
        learner->Start();
        delete learner;
        std::cout << "✓ Stage " << currentStage << " complete!\n" << std::endl;
    }

    std::cout << "\n🎉 ALL STAGES COMPLETE!" << std::endl;
    return EXIT_SUCCESS;
}
