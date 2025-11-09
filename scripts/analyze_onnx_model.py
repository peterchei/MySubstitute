"""
Analyze crossface_simswap.onnx model structure to understand input/output requirements.
"""

import onnx
import sys

def analyze_onnx_model(model_path):
    """Load and analyze ONNX model structure"""
    try:
        model = onnx.load(model_path)
        
        print("=" * 70)
        print(f"ONNX Model Analysis: {model_path}")
        print("=" * 70)
        
        # Get model inputs
        print("\n📥 MODEL INPUTS:")
        print("-" * 70)
        for i, input_tensor in enumerate(model.graph.input):
            print(f"\nInput {i+1}: {input_tensor.name}")
            print(f"  Type: {input_tensor.type.tensor_type.elem_type}")
            
            # Get shape
            shape = []
            for dim in input_tensor.type.tensor_type.shape.dim:
                if dim.dim_value:
                    shape.append(dim.dim_value)
                elif dim.dim_param:
                    shape.append(dim.dim_param)
                else:
                    shape.append('?')
            print(f"  Shape: {shape}")
        
        # Get model outputs
        print("\n📤 MODEL OUTPUTS:")
        print("-" * 70)
        for i, output_tensor in enumerate(model.graph.output):
            print(f"\nOutput {i+1}: {output_tensor.name}")
            print(f"  Type: {output_tensor.type.tensor_type.elem_type}")
            
            # Get shape
            shape = []
            for dim in output_tensor.type.tensor_type.shape.dim:
                if dim.dim_value:
                    shape.append(dim.dim_value)
                elif dim.dim_param:
                    shape.append(dim.dim_param)
                else:
                    shape.append('?')
            print(f"  Shape: {shape}")
        
        print("\n" + "=" * 70)
        print("✅ Analysis complete!")
        print("=" * 70)
        
        return True
        
    except Exception as e:
        print(f"❌ Error analyzing model: {e}")
        return False

if __name__ == '__main__':
    model_path = '../models/crossface_simswap.onnx'
    
    if len(sys.argv) > 1:
        model_path = sys.argv[1]
    
    print(f"\nAnalyzing: {model_path}\n")
    analyze_onnx_model(model_path)
