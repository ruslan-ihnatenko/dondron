# DonDron URDF frames

## Camera / perception

| Frame | Role |
|-------|------|
| `camera_link` | Camera housing, rigidly attached to `base_link` (forward, +X) |
| `camera_optical_frame` | Standard optical frame for vision messages (REP-103) |

Future `dondron_perception` publishes `/camera/image_raw` with:

```text
header.frame_id: camera_optical_frame
```

Use `camera_optical_frame` in `Detection2DArray` and TF lookups unless a node explicitly documents another frame.

## Expand path

Processed URDF for tools that do not read xacro:

```bash
xacro dondron.urdf.xacro > dondron.urdf
```
