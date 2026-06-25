# Main PC setup — QGroundControl & Docker (Mint 22)

Linux Mint 22 (Noble base). Run **sudo steps yourself**; non-sudo steps are already done where noted.

## QGroundControl

**Already done (no sudo):**

- AppImage: `~/Applications/QGroundControl/QGroundControl-x86_64.AppImage`
- Launcher (libmount workaround): `~/Applications/QGroundControl/run-qgc.sh`

### Step Q1 — dependencies (you run)

```bash
sudo apt update
sudo apt install -y \
  gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl \
  python3-gi python3-gst-1.0 libfuse2 \
  libxcb-xinerama0 libxkbcommon-x11-0 libxcb-cursor-dev

# USB serial for future Pixhawk / radio (log out & back in after this)
sudo usermod -aG dialout "$USER"

# Optional: stop ModemManager grabbing serial ports
sudo systemctl mask --now ModemManager.service
```

Log out and back in (or reboot) so `dialout` group applies.

### Step Q2 — launch

```bash
~/Applications/QGroundControl/run-qgc.sh
```

With PX4 SITL running, QGC connects via MAVLink on localhost UDP. Preflight `No connection to the GCS` should disappear.

---

## Docker Engine + NVIDIA GPU

### Step D1 — Docker Engine (you run)

Mint 22 uses Ubuntu **noble** packages (not `zena`):

```bash
sudo apt install -y ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu noble stable" \
  | sudo tee /etc/apt/sources.list.d/docker.list

sudo apt update
sudo apt install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

sudo usermod -aG docker "$USER"
```

Log out and back in so `docker` group applies (or run `newgrp docker` in the current terminal).

Verify:

```bash
docker run --rm hello-world
```

### Step D2 — NVIDIA Container Toolkit (you run)

```bash
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey \
  | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg

curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list \
  | sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' \
  | sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list

sudo apt update
sudo apt install -y nvidia-container-toolkit
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
```

GPU smoke test:

```bash
docker run --rm --gpus all nvidia/cuda:12.0-base nvidia-smi
```

### Step D3 — dondron dev image (after Docker works)

```bash
cd ~/Projects/dondron
docker compose -f docker/docker-compose.yml build
docker compose -f docker/docker-compose.yml run --rm dev
# inside container: ros2 doctor && colcon build
```

---

## Related

- [sil-bridge.md](sil-bridge.md) — PX4 SITL + uXRCE-DDS
- [docker.md](docker.md) — daily Docker / Dev Container usage
- Vault: `01_Projects/Robotics/DonDron/Tasks/mint22-setup-docker-gpu.md`
