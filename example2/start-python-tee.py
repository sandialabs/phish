import phish.school
import sys

filegen = phish.school.create_minnows("filegen", ["python", "filegen.py"] + sys.argv[1:])
print1 = phish.school.create_minnows("print", ["python", "print.py"])
print2 = phish.school.create_minnows("print", ["python", "print.py"])

phish.school.all_to_all(filegen, 0, phish.school.BROADCAST, 0, print1)
phish.school.all_to_all(filegen, 0, phish.school.BROADCAST, 0, print2)
phish.school.start()

