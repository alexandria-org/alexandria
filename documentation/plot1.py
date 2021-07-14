
import matplotlib.pyplot as plt

def rank(x):
	return max(x - 0.2, 0.2/100.0)

plt.plot([0.0, 0.1, 0.2, 0.3], [rank(0.0), rank(0.1), rank(0.2), rank(0.3)])
plt.ylabel('rank(link_source, link_target)')
plt.xlabel('harmonic(domain(link_source))')

plt.annotate('minimum value for link = 0.002', xy=(0.05, 0.002), xytext=(0.1, 0.04),
arrowprops=dict(facecolor='black', shrink=0.05),
)

plt.show()
