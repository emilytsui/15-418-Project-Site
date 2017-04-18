import random
from random import shuffle
import math

def Test():
	output = open("chunked_test_all.txt", "w")
	numVals = 250000
	lookupRatio = 33 # out of 100
	insertRatio = 33 # out of 100
	numDelete = numVals
	vals = []
	vals = generateKeyVals(numVals, vals)

	# deleteVals = vals
	# shuffle(deleteVals)
	inserted = []
	for i in range(numVals):
		r = random.randint(1, 100)
		if (len(inserted) == 0 or r <= insertRatio):
			newVal = vals[i]
			output.write("I " + str(newVal[0]) + " " + str(newVal[1]) + "\n")
			inserted = inserted + [newVal]
		elif (r - insertRatio <= lookupRatio):
			index = random.randint(0, len(inserted) - 1)
			output.write("L " + str(inserted[index][0]) + " " + str(inserted[index][1]) + "\n")
		else:
			index = random.randint(0, len(inserted) - 1)
			output.write("D " + str(inserted[index][0]) + " " + str(inserted[index][1]) + "\n")
			del(inserted[index])

	output.close()

def generateKeyVals(numVals, vals):
	for i in range(numVals):
		newVal = (random.randint(0, 2147483647), random.randint(0, 2147483647))
		vals = vals + [newVal]
	return vals

Test()
