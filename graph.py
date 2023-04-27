import matplotlib.pyplot as plt
import numpy as np

# data to plot
data1 = [95.9367, 93.5762, 97.2639, 92.069, 91.8443, 91.8786, 91.9025, 94.4396]
data2 = [95.509, 93.8071, 97.4218, 92.1824, 91.8938, 91.8958, 91.9367, 94.811]
data3 = [95.3002, 92.8151, 97.4222, 92.1862, 91.9103, 91.8988, 91.937, 93.8158]
# data1 = [6.6842, 9.4818, 6.4041, 11.9119, 12.3471, 12.3628, 12.2332, 8.31697]
# data2 = [6.67663, 9.4617, 6.0346, 11.9106, 12.3061, 12.359, 12.1922, 8.28723]
# data3 = [7.73113, 10.6052, 6.03353, 11.8529, 12.2911, 12.3551, 12.1145, 8.25013]
# create a bar plot
index = np.arange(len(data1))
width = 0.35
plt.bar(index, data1, width, label='HASHED_PERCEPTRON')
plt.bar(index + width, data2, width, label='TAGE_PREDICTOR')
plt.bar(index + 2*width - 0.1, data3, width, label='LTAGE_PREDICTOR')

# add some labels and titles
plt.xlabel('TRACES')
plt.ylabel('ACCURACY IN %')
plt.title('Comparison of HASHED_PERCEPTRON, TAGE_PREDICTOR, LTAGE_PREDICTOR')
plt.xticks(index + width / 2 + 0.15, ('low-1186B', 'med-109B', 'med-137B', 'med-267B', 'med-831B', 'med-1246B', 'med-1463B', 'low-299B'))
plt.legend()

# display the plot
plt.show()
