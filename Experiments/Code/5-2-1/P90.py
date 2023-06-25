import matplotlib.pyplot as plt
import numpy as np


data1 = [33.57, 35.3157520877937, 6.17597616722379]
data2 = [33.47, 35.2874851185677, 5.63588773702262]
data3 = [52.54, 52.4886349307177, 28.129957567062]
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
plt.savefig(r"P90.svg")

plt.show()