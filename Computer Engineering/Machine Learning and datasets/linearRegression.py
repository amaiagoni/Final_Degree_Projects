import numpy as np
from sklearn.linear_model import LinearRegression
import json
import matplotlib.pyplot as plt

file= open('noise_measurements.json',)
data = json.load(file)
mobile_sonometer = []
zaratamap_sonometer = []
for measure in data:
    mobile_sonometer.append(data[measure][0]["statement_id"])
    values_measured = []
    for m in data[measure][0]['series'][0]['values']:
        values_measured.append(m[2])
    zaratamap_sonometer.append(np.mean(np.array(values_measured)))

Z = [x for _,x in sorted(zip(mobile_sonometer,zaratamap_sonometer))]
mobile_sonometer = sorted(mobile_sonometer)
zaratamap_sonometer = Z

print(mobile_sonometer)
print(zaratamap_sonometer)

x = np.array(zaratamap_sonometer).reshape((-1, 1))
y = np.array(mobile_sonometer)

model = LinearRegression().fit(x, y)

r_sq = model.score(x, y)
print('coefficient of determination:', r_sq)

print('intercept:', model.intercept_)
print('slope:', model.coef_)

# Visualization of graph

plot_x = np.arange(0,1.6,0.01)
plot_y = model.coef_*plot_x+model.intercept_
plt.plot(plot_x, plot_y)
Z = [x for _,x in sorted(zip(zaratamap_sonometer,mobile_sonometer))]
zaratamap_sonometer = sorted(zaratamap_sonometer)
mobile_sonometer = Z
plt.scatter(zaratamap_sonometer, mobile_sonometer, c="green")
plt.ylabel('Noise in decibels')
plt.xlabel('Microphone measurement')
plt.show()