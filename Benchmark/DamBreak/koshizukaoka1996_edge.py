#!/usr/bin/env python
#-*- coding:utf-8 -*-

import sys
import os
import csv
import math
import numpy
import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot
import joblib

def output(dirname, filename):
	with open(dirname + "/" + filename, "r") as f:
		data = [[float(line["x"]), int(line["Type"])] for line in csv.DictReader(f, skipinitialspace="True")]

	edge = max([d[0] for d in data if d[1] == 0])

	return [edge]

def main(dirname, L, dt, g):
	# Martin & Moyce(1952), 1.125in
	with open("martin_moyce_1952_1125.csv") as f:
		data = [[float(line["t"]), float(line["z"])] for line in csv.DictReader(f)]
	data = numpy.array(data).T
	pyplot.plot(data[0], data[1], "o", label="Exp: Martin & Moyce(1952), 1.125in")

	# Martin & Moyce(1952), 2.25in
	with open("martin_moyce_1952_225.csv") as f:
		data = [[float(line["t"]), float(line["z"])] for line in csv.DictReader(f)]
	data = numpy.array(data).T
	pyplot.plot(data[0], data[1], "o", label="Exp: Martin & Moyce(1952), 2.25in")

	# Koshizuka et al.(1995)
	with open("koshizuka_etal_1995.csv") as f:
		data = [[float(line["t"]), float(line["z"])] for line in csv.DictReader(f)]
	data = numpy.array(data).T
	pyplot.plot(data[0], data[1], "s", label="Exp: Koshizuka et al.(1995)")

	# Hirt & Nichols(1981)
	with open("hirt_nichols_1981.csv") as f:
		data = [[float(line["t"]), float(line["z"])] for line in csv.DictReader(f)]
	data = numpy.array(data).T
	pyplot.plot(data[0], data[1], "--", label="SOVA-VOF: Hirt & Nichols(1981)")

	# Koshizuka & Oka(1996)
	with open("koshizuka_oka_1996.csv") as f:
		data = [[float(line["t"]), float(line["z"])] for line in csv.DictReader(f)]
	data = numpy.array(data).T
	pyplot.plot(data[0], data[1], "-", label="Origianl MPS: Koshizuka & Oka(1996)")

	# OpenMPS
	data = joblib.Parallel(n_jobs=-1, verbose=1)([joblib.delayed(output)(dirname, filename) for filename in sorted(os.listdir(dirname))])
#	data = [output(dirname, filename) for filename in sorted(os.listdir(dirname)) if filename.startswith("particles_")]
	data = numpy.array(data).T
	n = data.shape[1]
	maxT = n * dt
	t = numpy.array(range(n)) * dt
	tt = math.sqrt(2*g/L)
	z = data[0]
	pyplot.plot(t*tt, z/L, label="OpenMPS", color="red", linewidth=3)

	pyplot.xlim([0, 3.5])
	pyplot.ylim([1, 4])
	pyplot.xlabel("$t\sqrt{2g/L}$")
	pyplot.ylabel("$Z/L$")
	pyplot.grid(which="minor", color="gray", linestyle="dashed")
	pyplot.grid(which="major", color="black", linestyle="solid", b=True)
	pyplot.minorticks_on()
	pyplot.legend(prop={'size': 10})
	pyplot.savefig("edge.svg")
	pyplot.clf()

if __name__ == "__main__":
	main(sys.argv[1], float(sys.argv[2]), float(sys.argv[3]), float(sys.argv[4]))
