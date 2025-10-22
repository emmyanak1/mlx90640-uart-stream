import serial
import numpy as np
import matplotlib.pyplot as plt

# === CONFIG ===
PORT = '/dev/tty.usbmodem144203'
BAUD = 115200
FRAME_SIZE = 768
BYTES_PER_PIXEL = 2
START_BYTES = b'\xAA\x55'
END_BYTES = b'\x55\xAA'

# === INIT SERIAL ===
ser = serial.Serial(PORT, BAUD, timeout=2)
print(f"Listening on {PORT}...")

def read_frame():
    # Wait for start marker
    while True:
        if ser.read(2) == START_BYTES:
            break

    # Read payload
    payload = ser.read(FRAME_SIZE * BYTES_PER_PIXEL)

    # Read and verify end marker
    if ser.read(2) != END_BYTES:
        print("Invalid frame end!")
        return None

    # Convert to NumPy array of float32 temperatures
    frame = np.frombuffer(payload, dtype=np.int16).astype(np.float32) / 100.0
    if frame.size != FRAME_SIZE:
        print("Incomplete frame")
        return None
    return frame.reshape((24, 32))

# === READ 3 FRAMES ===

def capture_frames(n=3):
    frames = []
    while len(frames) < n:
        frame = read_frame()
        if frame is not None:
            frames.append(frame)
            print(f"Captured frame {len(frames)}/{n}")
    return frames


# === LIVE PLOT ===
fig, axes = plt.subplots(1, 3, figsize=(12, 5))
imgs = []

# Initialize plots
for ax in axes:
    img = ax.imshow(np.zeros((24, 32)), cmap='inferno', vmin=20, vmax=40)
    plt.colorbar(img, ax=ax, fraction=0.046, pad=0.04)
    imgs.append(img)
    ax.axis("off")

fig.suptitle("MLX90640 - 3 Captured Frames", fontsize=16)

plt.ion()
plt.show()

# === MAIN LOOP ===
while True:
    print("\nWaiting for next capture (press B1 on STM32)...")
    frames = capture_frames(3)

    for i in range(3):
        imgs[i].set_data(frames[i])
        imgs[i].set_clim(np.min(frames[i]), np.max(frames[i]))  # Auto-scale color
        imgs[i].changed()

    plt.pause(0.1)
