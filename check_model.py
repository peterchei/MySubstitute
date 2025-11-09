import onnx

model_path = "models/simswap.onnx"
model = onnx.load(model_path)

print(f"\n=== Model: {model_path} ===\n")
print("INPUTS:")
for inp in model.graph.input:
    shape = [d.dim_value if d.dim_value > 0 else d.dim_param for d in inp.type.tensor_type.shape.dim]
    print(f"  - {inp.name}: {shape}")

print("\nOUTPUTS:")
for out in model.graph.output:
    shape = [d.dim_value if d.dim_value > 0 else d.dim_param for d in out.type.tensor_type.shape.dim]
    print(f"  - {out.name}: {shape}")
