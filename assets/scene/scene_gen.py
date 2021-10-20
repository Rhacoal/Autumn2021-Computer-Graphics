import json
import math
import random


def rand_pos(l, r):
    return random.uniform(l, r), random.uniform(l, r), random.uniform(l, r)


def rand_dir():
    x, y, z = rand_pos(-1, 1)
    return normalize(x, y, z)


def length(x, y, z):
    return math.sqrt(x * x + y * y + z * z)


def normalize(x, y, z):
    _len = length(x, y, z)
    return x / _len, y / _len, z / _len


def quat(angle, center):
    real = math.cos(angle / 2)
    img = math.sin(angle / 2)
    i, j, k = normalize(*center)
    return real, img * i, img * j, img * k


boxes = []
for i in range(100):
    pos = rand_pos(-50, 50)
    if length(*pos) < 20:
        continue
    size = rand_pos(2, 6)
    color = normalize(*rand_pos(0.5, 1.0))
    rot = quat(random.uniform(0, 2 * math.pi), rand_dir())
    boxes.append({
        "type": "box",
        "position": pos,
        "size": size,
        "color": color,
        "rotation": rot,
        "shininess": random.uniform(1.0, 5.0),
    })
boxes.append({
    "type": "placeholder",
    "position": [0.0, 0.0, 0.0],
    "rotation": [1.0, 0.0, 0.0, 0.0],
    "children": [{
        "type": "box",
        "position": [10.0, 0.0, 0.0],
        "size": [2.0, 2.0, 2.0],
        "color": [0.7, 0.6, 1.0],
        "rotation": [1.0, 0.0, 0.0, 0.0],
        "shininess": 1.0
    }],
    "animation": {
        "type": "rotate",
        "axis": [0.0, 1.0, 0.0],
        "speed": 3.0,
    }
})

print(json.dumps(boxes))
with open("scene.json", "w") as f:
    json.dump({"objects": boxes}, f)
