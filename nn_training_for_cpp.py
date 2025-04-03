import torch
import torch.nn as nn
import numpy as np
import matplotlib.pyplot as plt
import os
os.environ["KMP_DUPLICATE_LIB_OK"] = "TRUE"

# Define a deeper network with multiple hidden layers
class DeepNN(nn.Module):
    def __init__(self):
        super(DeepNN, self).__init__()
        self.fc1 = nn.Linear(1, 20)  # Input -> Hidden 1
        self.fc2 = nn.Linear(20, 20)  # Hidden 1 -> Hidden 2
        self.fc3 = nn.Linear(20, 20)  # Hidden 2 -> Hidden 3
        self.fc4 = nn.Linear(20, 1)   # Hidden 3 -> Output

    def forward(self, x):
        x = torch.relu(self.fc1(x))
        x = torch.relu(self.fc2(x))
        x = torch.relu(self.fc3(x))
        x = self.fc4(x)
        return x

# Create model
model = DeepNN()

# Training data (y = x^2)
X_train = torch.tensor([[0], [1], [2], [3], [4], [5]], dtype=torch.float32)
Y_train = torch.tensor([[0], [1], [4], [9], [16], [25]], dtype=torch.float32)

# Optimizer & Loss function
optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
loss_fn = nn.MSELoss()

# Train the model
losses = []
for epoch in range(3000):
    optimizer.zero_grad()
    predictions = model(X_train)
    loss = loss_fn(predictions, Y_train)
    loss.backward()
    optimizer.step()
    losses.append(loss.item())

    if epoch % 500 == 0:
        print(f"Epoch {epoch}, Loss: {loss.item():.6f}")

# Plot Training Loss Curve
plt.figure(figsize=(8, 5))
plt.plot(losses, label="Training Loss", color="blue")
plt.xlabel("Epochs")
plt.ylabel("Loss")
plt.title("Training Loss Curve")
plt.legend()
plt.show()

# Visualize Predictions vs Actual Data
X_test = torch.linspace(-1, 6, 100).reshape(-1, 1)  # Test inputs
Y_test_pred = model(X_test).detach().numpy()  # Predictions

plt.figure(figsize=(8, 5))
plt.scatter(X_train, Y_train, color="red", label="Actual Data")
plt.plot(X_test, Y_test_pred, color="blue", label="Model Predictions")
plt.xlabel("X")
plt.ylabel("Y = X^2")
plt.title("Model Predictions vs Actual Data")
plt.legend()
plt.show()

# Export weights to a header file
W1 = model.fc1.weight.detach().numpy()
b1 = model.fc1.bias.detach().numpy()
W2 = model.fc2.weight.detach().numpy()
b2 = model.fc2.bias.detach().numpy()
W3 = model.fc3.weight.detach().numpy()
b3 = model.fc3.bias.detach().numpy()
W4 = model.fc4.weight.detach().numpy()
b4 = model.fc4.bias.detach().numpy()

with open("model_weights.h", "w") as f:
    f.write("// Automatically generated model weights\n\n")

    for i, (W, b) in enumerate([(W1, b1), (W2, b2), (W3, b3), (W4, b4)], start=1):
        f.write(f"const int fc{i}_in = {W.shape[1]}, fc{i}_out = {W.shape[0]};\n")
        f.write(f"const float W{i}[fc{i}_out][fc{i}_in] = {{\n")
        for row in W:
            row_str = ", ".join(f"{val:.6f}" for val in row)
            f.write("  { " + row_str + " },\n")
        f.write("};\n\n")
        f.write(f"const float b{i}[fc{i}_out] = {{ " + ", ".join(f"{val:.6f}" for val in b) + " };\n\n")

print("Model weights exported to model_weights.h")
