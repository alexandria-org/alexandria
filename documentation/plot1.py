
import matplotlib.pyplot as plt

plt.plot([-1000000, 0, 10000000, 11000000], [1000, 1000, 10000000, 10000000])
plt.axis([-1000000, 11000000, -1000000, 11000000])
plt.ylabel('R(L1, L2)')
plt.xlabel('H(D1) - H(D2)')
plt.show()
