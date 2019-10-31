import matplotlib.pyplot as plt
import numpy as np


font = {'family' : 'normal',
        # 'weight' : 'bold',
        'size'   : 18}
font_label = {'family' : 'normal',
        # 'weight' : 'bold',
        'size'   : 18}

plt.rc('font', **font)

x = [1,10,20,30,70,80,90,100,120,200,300,400]
y = [259.333,76.387, 33.520, 34.1, 34.682, 29.601, 30.93, 32.356, 33.608, 32.513, 33.328, 32.425]
plt.plot(x, y)

# plt.xticks(x, ('0%', '1%', '2%', '5%', '10%', '20%'))
axs = plt.gca()
axs.set_ylabel('Transfer Time (seconds)')
axs.set_xlabel('Starting Burst')
axs.grid('on',axis='y')

plt.savefig('ds3.png', format='png', dpi=300, bbox_inches='tight')
plt.show()
