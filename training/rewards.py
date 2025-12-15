"""
Reward Functions for 7-Stage Curriculum Training
Automatically adjusts rewards based on current training stage
"""

import numpy as np
from typing import Dict, Any


class CurriculumRewardFunction:
    """
    Multi-stage reward function that adapts to curriculum learning
    """
    
    def __init__(self, stage_config: Dict[str, Any]):
        self.stage = stage_config['name']
        self.weights = stage_config['reward_weights']
        
    def calculate_reward(self, player_data: Dict, game_state: Dict, 
                        prev_action: Dict, is_final: bool) -> float:
        """
        Calculate reward based on current stage and game state
        
        Args:
            player_data: Current player state
            game_state: Current game state  
            prev_action: Previous action taken
            is_final: Whether episode is ending
            
        Returns:
            Total reward for this timestep
        """
        reward = 0.0
        
        # Stage 1: Ball Contact & Awareness
        if self.stage == "ball_contact":
            reward += self._ball_contact_rewards(player_data, game_state)
            
        # Stage 2: Goal Shooting
        elif self.stage == "goal_shooting":
            reward += self._shooting_rewards(player_data, game_state)
            
        # Stage 3: Power & Accuracy
        elif self.stage == "power_accuracy":
            reward += self._power_accuracy_rewards(player_data, game_state)
            
        # Stage 4: Aerial Fundamentals
        elif self.stage == "aerial_basics":
            reward += self._aerial_basic_rewards(player_data, game_state)
            
        # Stage 5: Air Dribbles
        elif self.stage == "air_dribbles":
            reward += self._air_dribble_rewards(player_data, game_state)
            
        # Stage 6: Double Taps
        elif self.stage == "double_taps":
            reward += self._double_tap_rewards(player_data, game_state)
            
        # Stage 7: Pro 2v2 Game Sense
        elif self.stage == "pro_2v2_gamesense":
            reward += self._pro_gamesense_rewards(player_data, game_state)
        
        # Universal rewards (apply to all stages)
        reward += self._universal_rewards(player_data, game_state, is_final)
        
        return reward
    
    # ========================================================================
    # STAGE-SPECIFIC REWARD FUNCTIONS
    # ========================================================================
    
    def _ball_contact_rewards(self, player, state) -> float:
        """Stage 1: Encourage any ball contact"""
        reward = 0.0
        
        # Ball touch
        if player.get('ball_touched', False):
            reward += self.weights.get('ball_touch', 5.0)
        
        # Facing ball
        if self._is_facing_ball(player, state):
            reward += self.weights.get('facing_ball', 0.1)
        
        # Boost collection
        if player.get('boost_collected', False):
            reward += self.weights.get('boost_pickup', 0.5)
            
        return reward
    
    def _shooting_rewards(self, player, state) -> float:
        """Stage 2: Encourage shooting towards goal"""
        reward = 0.0
        
        # Ball touch (reduced from stage 1)
        if player.get('ball_touched', False):
            reward += self.weights.get('ball_touch', 2.0)
        
        # Shot towards goal
        if self._shot_towards_goal(player, state):
            reward += self.weights.get('shot_towards_goal', 10.0)
            
            # Extra reward if on target
            if self._shot_on_target(player, state):
                reward += self.weights.get('shot_on_target', 20.0)
        
        # Penalty for wrong direction
        if self._shot_towards_own_goal(player, state):
            reward += self.weights.get('own_goal_penalty', -50.0)
            
        return reward
    
    def _power_accuracy_rewards(self, player, state) -> float:
        """Stage 3: Encourage powerful, accurate shots"""
        reward = 0.0
        
        # Power shot (scaled by ball velocity)
        if player.get('ball_touched', False):
            ball_speed = np.linalg.norm(state['ball']['velocity'])
            power_reward = (ball_speed / 100.0) * self.weights.get('shot_power', 15.0)
            reward += power_reward
        
        # Shot accuracy (hitting corners/top of net)
        if self._accurate_shot(player, state):
            reward += self.weights.get('shot_accuracy', 10.0)
        
        # Dribbling
        if self._is_dribbling(player, state):
            reward += self.weights.get('dribble', 2.0)
        
        # Boost management
        boost_ratio = player.get('boost', 0) / 100.0
        if 0.3 < boost_ratio < 0.7:  # Encourage maintaining mid-level boost
            reward += self.weights.get('boost_management', 0.5)
            
        return reward
    
    def _aerial_basic_rewards(self, player, state) -> float:
        """Stage 4: Encourage aerial play"""
        reward = 0.0
        
        # Aerial touch
        if player.get('is_aerial', False) and player.get('ball_touched', False):
            base_reward = self.weights.get('aerial_touch', 15.0)
            
            # Height bonus
            height = player['position'][2]
            height_multiplier = min(height / 500.0, 2.0)  # Cap at 2x
            reward += base_reward * height_multiplier
        
        # Fast aerial detection
        if self._is_fast_aerial(player):
            reward += self.weights.get('fast_aerial', 10.0)
        
        # Landing recovery
        if player.get('just_landed', False) and player.get('wheels_on_ground', True):
            reward += self.weights.get('landing_recovery', 2.0)
        
        # Penalty for missing aerials
        if player.get('aerial_attempted', False) and not player.get('ball_touched', False):
            reward += self.weights.get('aerial_miss_penalty', -5.0)
            
        return reward
    
    def _air_dribble_rewards(self, player, state) -> float:
        """Stage 5: Encourage air dribbles"""
        reward = 0.0
        
        # Consecutive aerial touches
        consecutive_touches = player.get('consecutive_aerial_touches', 0)
        if consecutive_touches > 1:
            touch_reward = self.weights.get('consecutive_touches', 15.0)
            reward += touch_reward * (consecutive_touches ** 1.5)  # Exponential scaling
        
        # Maintaining ball proximity in air
        if player.get('is_aerial', False):
            distance_to_ball = self._distance_to_ball(player, state)
            if distance_to_ball < 200:  # Close to ball
                reward += self.weights.get('air_dribble_proximity', 5.0)
        
        # Air roll control
        if player.get('using_air_roll', False):
            reward += self.weights.get('air_roll_control', 3.0)
            
        return reward
    
    def _double_tap_rewards(self, player, state) -> float:
        """Stage 6: Encourage double taps and wall play"""
        reward = 0.0
        
        # Backboard touch
        if self._is_backboard_touch(player, state):
            reward += self.weights.get('backboard_touch', 8.0)
        
        # Double tap setup (ball hits backboard, player follows)
        if self._is_double_tap_setup(player, state):
            reward += self.weights.get('double_tap_setup', 20.0)
        
        # Wall aerial
        if player.get('was_on_wall', False) and player.get('is_aerial', False):
            reward += self.weights.get('wall_aerial', 10.0)
        
        # Rebound read
        if self._good_rebound_read(player, state):
            reward += self.weights.get('rebound_read', 10.0)
            
        return reward
    
    def _pro_gamesense_rewards(self, player, state) -> float:
        """Stage 7: Encourage pro-level 2v2 decision making"""
        reward = 0.0
        
        # Good positioning
        if self._good_position(player, state):
            reward += self.weights.get('positioning', 2.0)
        
        # Proper rotation
        if self._proper_rotation(player, state):
            reward += self.weights.get('rotation', 3.0)
        
        # Defensive save
        if player.get('made_save', False):
            reward += self.weights.get('defensive_save', 50.0)
        
        # Passing
        if self._good_pass(player, state):
            reward += self.weights.get('passing', 75.0)
        
        # Assist
        if player.get('assist', False):
            reward += self.weights.get('assist', 75.0)
        
        # Penalty for ball chasing
        if self._is_ball_chasing(player, state):
            reward += self.weights.get('ball_chasing_penalty', -5.0)
        
        # Penalty for being out of position
        if not self._good_position(player, state):
            reward += self.weights.get('out_of_position_penalty', -2.0)
        
        # Boost starvation (denying opponent boost)
        if self._boost_starve(player, state):
            reward += self.weights.get('boost_starve', 5.0)
        
        # Fake challenge
        if self._fake_challenge(player, state):
            reward += self.weights.get('fake_challenge', 10.0)
            
        return reward
    
    # ========================================================================
    # UNIVERSAL REWARDS (ALL STAGES)
    # ========================================================================
    
    def _universal_rewards(self, player, state, is_final) -> float:
        """Rewards that apply to all stages"""
        reward = 0.0
        
        # Goal scored
        if player.get('goal_scored', False):
            reward += self.weights.get('goal', 100.0)
            
            # Bonus for special goals
            if self.stage == "air_dribbles" and player.get('consecutive_aerial_touches', 0) > 2:
                reward += self.weights.get('air_dribble_goal', 200.0)
            elif self.stage == "double_taps" and player.get('double_tap', False):
                reward += self.weights.get('double_tap_goal', 250.0)
        
        # Goal conceded
        if player.get('goal_conceded', False):
            reward -= 100.0
        
        # Win/Loss (Stage 7 only)
        if self.stage == "pro_2v2_gamesense" and is_final:
            if state.get('winner') == player['team']:
                reward += self.weights.get('win', 300.0)
            else:
                reward += self.weights.get('loss', -300.0)
                
        return reward
    
    # ========================================================================
    # HELPER FUNCTIONS
    # ========================================================================
    
    def _is_facing_ball(self, player, state) -> bool:
        """Check if player is facing the ball"""
        # Simplified - implement proper vector math
        return True  # Placeholder
    
    def _shot_towards_goal(self, player, state) -> bool:
        """Check if shot is going towards opponent goal"""
        return True  # Placeholder
    
    def _shot_on_target(self, player, state) -> bool:
        """Check if shot is on target"""
        return True  # Placeholder
    
    def _shot_towards_own_goal(self, player, state) -> bool:
        """Check if shot is going towards own goal"""
        return False  # Placeholder
    
    def _accurate_shot(self, player, state) -> bool:
        """Check if shot is accurate (corners/top net)"""
        return True  # Placeholder
    
    def _is_dribbling(self, player, state) -> bool:
        """Check if player is dribbling"""
        return False  # Placeholder
    
    def _is_fast_aerial(self, player) -> bool:
        """Check if player is performing fast aerial"""
        return False  # Placeholder
    
    def _distance_to_ball(self, player, state) -> float:
        """Calculate distance to ball"""
        player_pos = np.array(player['position'])
        ball_pos = np.array(state['ball']['position'])
        return np.linalg.norm(player_pos - ball_pos)
    
    def _is_backboard_touch(self, player, state) -> bool:
        """Check if touch occurred on backboard"""
        return False  # Placeholder
    
    def _is_double_tap_setup(self, player, state) -> bool:
        """Check if player is setting up double tap"""
        return False  # Placeholder
    
    def _good_rebound_read(self, player, state) -> bool:
        """Check if player read rebound well"""
        return False  # Placeholder
    
    def _good_position(self, player, state) -> bool:
        """Check if player is in good position"""
        return True  # Placeholder
    
    def _proper_rotation(self, player, state) -> bool:
        """Check if player is rotating properly"""
        return True  # Placeholder
    
    def _good_pass(self, player, state) -> bool:
        """Check if player made good pass"""
        return False  # Placeholder
    
    def _is_ball_chasing(self, player, state) -> bool:
        """Check if player is ball chasing"""
        return False  # Placeholder
    
    def _boost_starve(self, player, state) -> bool:
        """Check if player is denying opponent boost"""
        return False  # Placeholder
    
    def _fake_challenge(self, player, state) -> bool:
        """Check if player successfully faked challenge"""
        return False  # Placeholder


# ============================================================================
# REWARD FUNCTION FACTORY
# ============================================================================

def create_reward_function(stage_config: Dict[str, Any]):
    """
    Factory function to create appropriate reward function for stage
    
    Args:
        stage_config: Configuration for current training stage
        
    Returns:
        CurriculumRewardFunction instance
    """
    return CurriculumRewardFunction(stage_config)
