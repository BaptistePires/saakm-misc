from sys import argv

id_= int(argv[1])

with open("/tmp/%d" % id_, "w") as f:
    for i in range(1, 100):
        f.write("%d\n" % i)


res = 0
with open("/tmp/%d" % id_, "r") as f:
    tmp = [int(l.strip()) for l in f.readlines()]
    res = sum(tmp)

print(res)
