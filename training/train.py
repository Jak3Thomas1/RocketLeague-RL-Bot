"""
GigaLearnCPP Master Training Script
7-Stage Curriculum Learning for 2v2 Rocket League Bot

Usage:
    python train.py                    # Start from Stage 1
    python train.py --stage 3          # Resume from specific stage
    python train.py --resume           # Resume from last checkpoint
"""

import os
import sys
import yaml
import argparse
import torch
from pathlib import Path
from datetime import datetime

# Add GigaLearnCPP to path
sys.path.append(str(Path(__file__).parent.parent / "GigaLearnCPP"))

# Import GigaLearnCPP modules
try:
    import GigaLearnCPP as GGL
    from rewards import create_reward_function
except ImportError as e:
    print(f"Error importing GigaLearnCPP: {e}")
    print("Make sure GigaLearnBot.exe is compiled and Python bindings are available")
    sys.exit(1)


class CurriculumTrainer:
    """
    Manages 7-stage curriculum training for Rocket League bot
    """
    
    def __init__(self, config_path: str = "config.yaml"):
        """Initialize trainer with configuration"""
        self.config = self._load_config(config_path)
        self.current_stage = 1
        self.total_timesteps = 0
        
        # Setup directories
        self._setup_directories()
        
        # Initialize visualization if enabled
        if self.config['visualization']['enabled']:
            self._setup_visualization()
        
        print(f"🚀 Curriculum Trainer Initialized")
        print(f"📁 Save Directory: {self.config['save_dir']}")
        print(f"🎮 Training for 2v2 Pro-Level Bot")
        print(f"⚡ Device: {self.config['device']}")
        
    def _load_config(self, config_path: str) -> dict:
        """Load YAML configuration"""
        with open(config_path, 'r') as f:
            return yaml.safe_load(f)
    
    def _setup_directories(self):
        """Create necessary directories"""
        Path(self.config['save_dir']).mkdir(parents=True, exist_ok=True)
        Path(self.config['log_dir']).mkdir(parents=True, exist_ok=True)
        
    def _setup_visualization(self):
        """Setup RocketSimVis connection"""
        try:
            vis_config = self.config['visualization']
            print(f"🎨 Connecting to RocketSimVis at {vis_config['host']}:{vis_config['port']}")
            # TODO: Implement actual connection
        except Exception as e:
            print(f"⚠️  Visualization setup failed: {e}")
            print("Continuing without visualization...")
    
    def train_all_stages(self, start_stage: int = 1):
        """
        Train through all curriculum stages
        
        Args:
            start_stage: Which stage to start from (1-7)
        """
        stages = ['stage1', 'stage2', 'stage3', 'stage4', 'stage5', 'stage6', 'stage7']
        
        for stage_num in range(start_stage, 8):
            stage_key = stages[stage_num - 1]
            stage_config = self.config['curriculum'][stage_key]
            
            print(f"\n{'='*70}")
            print(f"🎯 STAGE {stage_num}/7: {stage_config['name'].upper()}")
            print(f"{'='*70}")
            print(f"Target Timesteps: {stage_config['timesteps']:,}")
            print(f"Learning Rate: {stage_config['learning_rate']}")
            
            # Train this stage
            success = self.train_stage(stage_num, stage_config)
            
            if success:
                print(f"✅ Stage {stage_num} completed successfully!")
                self._save_stage_checkpoint(stage_num, stage_config['name'])
            else:
                print(f"❌ Stage {stage_num} failed to meet success criteria")
                print("Consider adjusting hyperparameters or training longer")
                break
        
        print(f"\n🎉 Training Complete! Final model saved.")
        
    def train_stage(self, stage_num: int, stage_config: dict) -> bool:
        """
        Train a single curriculum stage
        
        Args:
            stage_num: Stage number (1-7)
            stage_config: Configuration for this stage
            
        Returns:
            True if stage completed successfully
        """
        # Create environment
        env = self._create_environment(stage_config)
        
        # Create reward function
        reward_fn = create_reward_function(stage_config)
        
        # Create/load model
        if stage_num == 1:
            model = self._create_model()
        else:
            model = self._load_previous_stage_model(stage_num - 1)
        
        # Setup learner
        learner_config = self._create_learner_config(stage_config)
        learner = GGL.PPOLearner(
            env_create_fn=env,
            config=learner_config,
            reward_fn=reward_fn
        )
        
        # Training loop
        print(f"\n⏳ Training Stage {stage_num}...")
        
        timesteps_trained = 0
        target_timesteps = stage_config['timesteps']
        
        try:
            while timesteps_trained < target_timesteps:
                # Train for checkpoint interval
                checkpoint_interval = self.config['checkpoints']['save_frequency']
                
                learner.learn(steps=checkpoint_interval)
                timesteps_trained += checkpoint_interval
                self.total_timesteps += checkpoint_interval
                
                # Print progress
                progress = (timesteps_trained / target_timesteps) * 100
                print(f"Progress: {progress:.1f}% ({timesteps_trained:,}/{target_timesteps:,} steps)")
                
                # Save checkpoint
                if timesteps_trained % checkpoint_interval == 0:
                    checkpoint_path = self._get_checkpoint_path(stage_num, timesteps_trained)
                    learner.save(checkpoint_path)
                    print(f"💾 Checkpoint saved: {checkpoint_path}")
        
        except KeyboardInterrupt:
            print(f"\n⚠️  Training interrupted by user")
            checkpoint_path = self._get_checkpoint_path(stage_num, timesteps_trained)
            learner.save(checkpoint_path)
            print(f"💾 Progress saved: {checkpoint_path}")
            return False
        
        # Evaluate stage completion
        success = self._evaluate_stage(learner, stage_config)
        
        return success
    
    def _create_environment(self, stage_config: dict):
        """Create environment function for GigaLearnCPP"""
        def env_create_fn(env_id: int):
            # TODO: Implement proper environment creation based on stage
            # This will use GigaLearnCPP's environment system
            pass
        
        return env_create_fn
    
    def _create_model(self):
        """Create initial model with config architecture"""
        ppo_config = self.config['ppo']
        
        # TODO: Implement model creation with GigaLearnCPP
        # This will create the policy and critic networks
        pass
    
    def _load_previous_stage_model(self, prev_stage: int):
        """Load model from previous stage to continue training"""
        stage_name = self.config['curriculum'][f'stage{prev_stage}']['name']
        model_path = Path(self.config['save_dir']) / f"stage{prev_stage}_{stage_name}_final.pt"
        
        if not model_path.exists():
            print(f"⚠️  Previous stage model not found: {model_path}")
            print("Creating new model instead...")
            return self._create_model()
        
        print(f"📂 Loading model from Stage {prev_stage}: {model_path}")
        # TODO: Implement model loading
        pass
    
    def _create_learner_config(self, stage_config: dict) -> dict:
        """Create GigaLearnCPP learner configuration"""
        ppo_config = self.config['ppo']
        
        config = {
            'learning_rate': stage_config['learning_rate'],
            'batch_size': ppo_config['batch_size'],
            'minibatch_size': ppo_config['minibatch_size'],
            'epochs': ppo_config['epochs'],
            'gamma': ppo_config['gamma'],
            'gae_lambda': ppo_config['gae_lambda'],
            'clip_range': ppo_config['clip_range'],
            'entropy_coef': ppo_config['entropy_coef'],
            'value_loss_coef': ppo_config['value_loss_coef'],
            'max_grad_norm': ppo_config['max_grad_norm'],
            'num_workers': self.config['num_workers'],
            'device': self.config['device']
        }
        
        return config
    
    def _evaluate_stage(self, learner, stage_config: dict) -> bool:
        """
        Evaluate if stage has met success criteria
        
        Args:
            learner: Trained learner instance
            stage_config: Configuration with success criteria
            
        Returns:
            True if criteria met
        """
        if 'success_criteria' not in stage_config:
            return True  # No criteria specified, assume success
        
        print("\n📊 Evaluating stage completion...")
        
        # TODO: Implement actual evaluation
        # Run test episodes and check metrics against criteria
        
        criteria = stage_config['success_criteria']
        print(f"Success Criteria: {criteria}")
        
        # Placeholder - actual implementation would test the bot
        return True
    
    def _save_stage_checkpoint(self, stage_num: int, stage_name: str):
        """Save final checkpoint for completed stage"""
        checkpoint_path = Path(self.config['save_dir']) / f"stage{stage_num}_{stage_name}_final.pt"
        
        # TODO: Implement checkpoint saving
        print(f"💾 Stage {stage_num} final model saved: {checkpoint_path}")
    
    def _get_checkpoint_path(self, stage_num: int, timesteps: int) -> str:
        """Generate checkpoint file path"""
        stage_name = self.config['curriculum'][f'stage{stage_num}']['name']
        filename = f"stage{stage_num}_{stage_name}_{timesteps//1000000}M.pt"
        return str(Path(self.config['save_dir']) / filename)


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='Train Rocket League 2v2 Bot')
    parser.add_argument('--config', type=str, default='config.yaml',
                       help='Path to configuration file')
    parser.add_argument('--stage', type=int, default=1, choices=range(1, 8),
                       help='Start from specific stage (1-7)')
    parser.add_argument('--resume', action='store_true',
                       help='Resume from last checkpoint')
    
    args = parser.parse_args()
    
    # Print header
    print("=" * 70)
    print("🚀 GigaLearnCPP - Rocket League 2v2 Bot Trainer")
    print("=" * 70)
    print(f"Configuration: {args.config}")
    print(f"Start Stage: {args.stage}")
    print(f"Device: {'CUDA (GPU)' if torch.cuda.is_available() else 'CPU'}")
    print(f"PyTorch Version: {torch.__version__}")
    print(f"CUDA Available: {torch.cuda.is_available()}")
    if torch.cuda.is_available():
        print(f"GPU: {torch.cuda.get_device_name(0)}")
        print(f"GPU Memory: {torch.cuda.get_device_properties(0).total_memory / 1e9:.2f} GB")
    print("=" * 70)
    
    # Verify GPU
    if not torch.cuda.is_available():
        print("\n⚠️  WARNING: CUDA not available! Training will be VERY slow on CPU.")
        response = input("Continue anyway? (y/N): ")
        if response.lower() != 'y':
            print("Exiting...")
            return
    
    # Initialize trainer
    trainer = CurriculumTrainer(config_path=args.config)
    
    # Start training
    try:
        trainer.train_all_stages(start_stage=args.stage)
    except Exception as e:
        print(f"\n❌ Training failed with error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
