"""Explicit commands and passive metrics for the owned original-client test mode."""
from dataclasses import asdict, dataclass
import math
import struct
from training_channel import ChannelError, ChannelNotReady, TrainingChannel


@dataclass(frozen=True)
class OriginalFrame:
    time: float
    location: tuple
    velocity: tuple
    camera: tuple
    rotation: tuple
    pitch_recoil: float
    yaw_recoil: float
    ammo: int
    max_ammo: int
    physics_calls: int
    native_rays: int
    capsule_half_height: float
    jump_velocity: float
    maximum_speed: float
    movement_type: int
    clock_step: int
    flags: int
    observer_error: int
    input_flags: int
    original_jump_height: float
    original_jump_time: float
    original_capsule_half_height: float
    equipped_movement_ratio: float
    shot_count: int
    shots: tuple
    observer_hooks: int
    shot_callbacks: int
    initial_snapshots: int
    invalid_payloads: int
    field_of_view: float
    original_input_observed: int
    original_primary_input: int
    original_gun_anim_instance: int
    original_gun_active_montage: int

    def as_dict(self):
        return asdict(self)


class OriginalProbe:
    """Stages7/9 accept expiring test commands; stage5 exposes observations only."""
    def __init__(self, channel: TrainingChannel):
        self.channel = channel
        self.sequence = 0
        self.aim_sequence = 0

    def read(self, attempts=12):
        buf = self.channel.buffer
        self.channel.validate_header()
        if struct.unpack_from('<I', buf, 760)[0] != 0x3152504f:
            raise ChannelNotReady('Original simulation metrics are not ready')
        for _ in range(attempts):
            before = struct.unpack_from('<I', buf, 768)[0]
            if not before or before & 1:
                self.channel.sleep(.0005)
                continue
            raw = bytes(buf[768:3584])
            if before != struct.unpack_from('<I', buf, 768)[0]:
                continue
            def unpack(fmt, offset):
                values = struct.unpack_from('<' + fmt, raw, offset-768)
                return values[0] if len(values) == 1 else values
            count, capacity = unpack('Q', 960), unpack('I', 968)
            if capacity != 32:
                raise ChannelError('Original shot ring capacity mismatch')
            shots = []
            for index in range(max(0, count-capacity), count):
                item = unpack('7diIiIQ', 1024+(index % capacity)*80)
                shots.append(dict(time=item[0], start=item[1:4], direction=item[4:7],
                                  native_index=item[7], flags=item[8], ammo=item[9],
                                  projectile_index=item[10], sequence=item[11]))
            values = [unpack('d', 776), unpack('3d', 784), unpack('3d', 808),
                      unpack('3d', 832), unpack('3d', 856), *unpack('2f', 880),
                      *unpack('2i', 888), *unpack('2Q', 896), *unpack('3f', 912),
                      unpack('I', 924), unpack('i', 928), *unpack('3I', 932),
                      *unpack('4f', 944), count, tuple(shots), *unpack('4I', 972),
                      unpack('f', 988), *unpack('2I',992), *unpack('2Q',1000)]
            result = OriginalFrame(*values)
            if not all(math.isfinite(x) for vector in (result.location, result.velocity,
                                                       result.camera, result.rotation) for x in vector):
                raise ChannelError('Nonfinite original-player observation')
            return result
        raise ChannelNotReady('No stable original-player metrics')

    def command(self, *, flags=8, forward=0., right=0., jump=False, walk=False, crouch=False):
        buf = self.channel.buffer
        stage=struct.unpack_from('<I', buf, 56)[0]
        if stage not in (7,9):
            raise ChannelError('Original test commands require explicit stage7 or stage9')
        if flags & ~(1023 if stage==9 else 15) or not all(math.isfinite(v) and abs(v) <= 1 for v in (forward, right)):
            raise ValueError('Invalid original-player test input')
        native_time = self.read().time
        self.sequence = ((self.sequence+2) & 0xfffffffe) or 2
        struct.pack_into('<I', buf, 20, self.sequence-1)
        struct.pack_into('<IffId', buf, 24, flags, forward, right,
                         int(jump) | (int(walk) << 1) | (int(crouch) << 2), native_time+.3)
        struct.pack_into('<I', buf, 20, self.sequence)

    def release(self):
        self.command(flags=0)

    def aim(self, *, pitch, yaw):
        """Stage9-only control rotation, consumed by the original Controller API."""
        buf = self.channel.buffer
        self.channel.validate_header()
        if struct.unpack_from('<I', buf, 56)[0] != 9:
            raise ChannelError('Original test aim requires explicit stage9')
        if not all(math.isfinite(v) for v in (pitch, yaw)) or abs(pitch) > 89 or abs(yaw) > 36000:
            raise ValueError('Invalid original-player test aim')
        expiry = self.read().time + .3
        self.aim_sequence = ((self.aim_sequence + 2) & 0xfffffffe) or 2
        struct.pack_into('<I', buf, 3588, self.aim_sequence - 1)
        struct.pack_into('<3d', buf, 3592, pitch, yaw, expiry)
        struct.pack_into('<I', buf, 3584, 0x314d4941)
        struct.pack_into('<I', buf, 3588, self.aim_sequence)
