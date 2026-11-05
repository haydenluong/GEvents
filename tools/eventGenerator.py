import sys


count = 100000
seed = 42
output = "data/events.tsv"

i = 1
while i < len(sys.argv):
    option = sys.argv[i]

    if option not in ["--count", "--seed", "--out"]:
        sys.exit("Unknown option: " + option)

    if i + 1 >= len(sys.argv):
        sys.exit("Missing value for " + option)

    value = sys.argv[i + 1]

    if option == "--out":
        output = value
    else:
        try:
            number = int(value)
        except ValueError:
            sys.exit(option + " must be a whole number")

        if option == "--count":
            count = number
        else:
            seed = number

    i += 2

if count <= 0:
    sys.exit("--count must be greater than 0")

print("Count:", count)
print("Seed:", seed)
print("Output:", output)
