import matplotlib.pyplot as plt
import numpy as np


data1 = [15.3717141538272, 8.58380426292458, 21.8145519679591]
data2 = [18.0236776800442, 9.64006750851789, 22.2154038165807]
data3 = [68.3632986656718, 74.3609146959268, 65.565609640873]
labels = ['25', '50', '86']

x = np.arange(len(labels))
width = 0.25

fig, ax = plt.subplots(figsize=(10,6))
ax.grid(True, axis='y', color = '#9E9E9E', clip_on = False)

plt.rcParams.update({'hatch.color' : 'white'})
rect1 = ax.bar(x-width, data1, width, color = '#EED5D2', label = 'Config. 1', zorder = 10, hatch = '/')
rect2 = ax.bar(x, data2, width, color = '#87CEEB', label = 'Config. 2', zorder = 10, hatch = '.')
rect3 = ax.bar(x+width, data3, width, color = '#F5DEB3', label = 'Config. 3', zorder = 10, hatch = '+')

plt.subplots_adjust(bottom=0.15, left=0.14)
ax.set_ylabel('Error Rate(%)', fontsize = 26, weight = 'bold')
ax.set_xticks(x, labels)
ax.set_xlabel('Arrive Rate(/s)', fontsize = 26, weight = 'bold')
plt.xticks(fontsize = 22)
plt.yticks(fontsize = 22)

ax.legend(loc = 'upper right', bbox_to_anchor=(0.87, 1.0), fontsize = 18)

fig.tight_layout()
plt.savefig(r"P999.svg")

plt.show()