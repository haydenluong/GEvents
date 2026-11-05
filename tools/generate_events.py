import sys
import random
import os
from datetime import date, timedelta


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

categories = {
    "social": ["friends", "games", "food", "music", "meet", "campus", "club", "fun"],
    "technology": ["code", "robotics", "software", "computer", "learn", "build", "workshop", "design"],
    "entrepreneurship": ["startup", "business", "pitch", "ideas", "funding", "team", "network", "build"],
    "academic": ["study", "research", "science", "math", "learn", "lecture", "writing", "workshop"],
    "sports": ["soccer", "basketball", "running", "fitness", "team", "games", "training", "outdoors"],
    "arts": ["painting", "dance", "music", "theater", "film", "design", "drawing", "workshop"],
    "career": ["jobs", "resume", "interview", "internship", "network", "skills", "workshop", "business"],
    "volunteering": ["service", "community", "cleanup", "donation", "food", "help", "outdoors", "team"]
}

locations = ["Student Union", "Library", "Recreation Center", "Campus Green", "Auditorium"]

random.seed(seed)

folder = os.path.dirname(output)
if folder != "":
    os.makedirs(folder, exist_ok=True)

with open(output, "w", encoding="utf-8", newline="") as file:
    file.write("id\ttitle\tdate\tlocation\tcategory\tdescription\tkeywords\n")

    for event_id in range(count):
        category = random.choice(list(categories))
        location = random.choice(locations)
        keyword_count = random.randint(3, 8)
        keywords = random.sample(categories[category], keyword_count)
        event_date = date(2026, 1, 1) + timedelta(days=random.randint(0, 364))
        title = category.capitalize() + " Event " + str(event_id)
        description = "Join a campus event about " + keywords[0] + "."

        fields = [str(event_id), title, str(event_date), location, category, description, ";".join(keywords)]
        file.write("\t".join(fields) + "\n")

print("Generated", count, "events in", output)
