#!/usr/bin/env python
#-*- coding:utf-8 -*-

import sys
import xml.etree.ElementTree as ET
from xml.dom import minidom

condition = { \
	"startTime": 0, 
    "endTime": 0.5,
    "outputInterval": 0.005,
    "eps": 1e-10,
}

environment = {\
	"l_0": 1e-3,
    "minStepCoountPerOutput": 10,
    "courant": 0.1,

    "g": 9.8,
    "rho": 998.20,
    "nu": 1.004e-6,
    "r_eByl_0": 2.4,
    "surfaceRatio": 0.95,
}

staticPressure ={\
	"width": 50, # ���q�̐�
	"height": 100,
	"wall": 110,
}

type = {\
	"IncompressibleNewton": 0, # �񈳏k���j���[�g�����́i���Ȃǁj
	"Wall": 1, # �ǖ�
	"Dummy": 2, # �_�~�[���q�i�ǖʕt�߂̗��q�����x���グ�邽�߂����Ɏg���闱�q�j
}


def main():
	openmps = ET.Element("openmps")

	# �v�Z�����̐���
	c = ET.SubElement(openmps, "condition")
	for key in condition:
		item = ET.SubElement(c, key)
		item.set("value", str(condition[key]))

	# �v�Z��Ԃ̊��l
	e = ET.SubElement(openmps, "environment")
	for key in environment:
		item = ET.SubElement(e, key)
		item.set("value", str(environment[key]))

	# ���q�̐���
	width =staticPressure["width"] 
	height = staticPressure["height"]
	l_0 = environment["l_0"]
	minX = sys.float_info.max
	minZ = sys.float_info.max
	maxX = sys.float_info.min
	maxZ = sys.float_info.min
	particlesCsv= ["Type, x, z, u, w, p, n\n"]
	# ��
	for i in range(0, width):
		for j in range(0, height):
			x = i * l_0
			z = j * l_0
			particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["IncompressibleNewton"], x, z))
			minX = min(minX, x)
			minZ = min(minZ, z)
			maxX = max(maxX, x)
			maxZ = max(maxZ, z)
	# ��
	for i in range(-1, width + 1):
		x = i * l_0
		z = -1 * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Wall"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = i * l_0
		z = -2 * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = i * l_0
		z = -3 * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = i * l_0
		z = -4 * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)
	# ����
	for j in range(0, height + 1):
		x = -1 * l_0
		z = j * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Wall"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = -2 * l_0
		z = j * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = -3 * l_0
		z = j * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = -4 * l_0
		z = j * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)
	# �E��
	for j in range(0, height + 1):
		x = (width + 0) * l_0
		z = j * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Wall"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = (width + 1) * l_0
		z = j * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = (width + 2) * l_0
		z = j * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)

		x = (width + 3) * l_0
		z = j * l_0
		particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
		minX = min(minX, x)
		minZ = min(minZ, z)
		maxX = max(maxX, x)
		maxZ = max(maxZ, z)
	# �l��
	for i in range(1, 4):
		for j in range(-4, 0):
			# ����
			x = (-1 - i) * l_0
			z = j * l_0
			particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
			minX = min(minX, x)
			minZ = min(minZ, z)
			maxX = max(maxX, x)
			maxZ = max(maxZ, z)
			# �E��
			x = (width + i) * l_0
			z = j * l_0
			particlesCsv.append("{0}, {1}, {2}, 0, 0, 0, 0\n".format(type["Dummy"], x, z))
			minX = min(minX, x)
			minZ = min(minZ, z)
			maxX = max(maxX, x)
			maxZ = max(maxZ, z)

	particles = ET.SubElement(openmps, "particles")
	particles.set("type", "csv")
	particles.text = "".join(particlesCsv)

	# �v�Z��Ԃ͈̔͂�ݒ�
	ET.SubElement(e, "minX").set("value", str(minX))
	ET.SubElement(e, "minZ").set("value", str(minZ))
	ET.SubElement(e, "maxX").set("value", str(maxX))
	ET.SubElement(e, "maxZ").set("value", str(maxZ))
	

	# �t�@�C���ɕۑ�
	xml = minidom.parseString(ET.tostring(openmps)).toprettyxml(indent="	")
	with open("test.xml", "w") as f:
		f.write(xml)

if __name__ == "__main__":
	main()