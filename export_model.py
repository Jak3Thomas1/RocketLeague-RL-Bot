# export_model.py
import torch
import onnx
from pathlib import Path

# Load your trained model
checkpoint_path = "path/to/your/checkpoint.pt"  # UPDATE THIS
checkpoint = torch.load(checkpoint_path)

# Get the policy network
model = checkpoint['model']  # Or however you access it
model.eval()

# Create dummy input (match your observation space)
# For RLGym this is usually 107 dimensional
dummy_input = torch.randn(1, 107)  # Adjust size to match your obs

# Export to ONNX
output_path = "rocket_league_bot.onnx"
torch.onnx.export(
    model,
    dummy_input,
    output_path,
    export_params=True,
    opset_version=11,
    do_constant_folding=True,
    input_names=['observations'],
    output_names=['actions'],
    dynamic_axes={
        'observations': {0: 'batch_size'},
        'actions': {0: 'batch_size'}
    }
)

print(f"Model exported to {output_path}")

# Verify it works
onnx_model = onnx.load(output_path)
onnx.checker.check_model(onnx_model)
print("ONNX model is valid!")