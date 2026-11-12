"""The RGB555 colors the engine currently uses."""


def values() -> list[int]:
    standard = [(0, 0, 0), (31, 0, 0), (0, 31, 0), (0, 0, 31),
                (31, 31, 0), (0, 31, 31), (31, 0, 31), (15, 15, 15),
                (31, 15, 0), (15, 0, 31)]
    channels = standard + [(5 * i % 32, 11 * i % 32, 17 * i % 32) for i in range(10, 21)]
    return [r | (g << 5) | (b << 10) for r, g, b in channels]


def derive() -> dict[str, bytes]:
    return {'generated/semantic_rgb.inc':
            ('static const uint16_t rgb[21]={' + ','.join(map(str, values())) + '};\n').encode()}
