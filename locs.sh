#!/bin/bash
cloc . | awk 'NR > 3' | awk 'NR==2 {print substr(/bin/bash, 22)}; NR>2 {print}'
cloc . | awk 'NR > 3' | awk 'NR==2 {print substr(/bin/bash, 22)}; NR>2 {print}' > locs.txt