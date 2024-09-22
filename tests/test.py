import cmake_example as m
import os
import sys
import numpy as np
import time

bs = sys.argv[1]
filepath = sys.argv[2]
os.environ["BATCH_SIZE"] = bs
os.environ["MP4_PATH"] = filepath

x = m.add(1, 2)

start = time.time()
x = m.multi_thread_loader()
end = time.time()

print(f"time is {end - start} seconds")
print(type(x))
print(len(x))
print(type(x[0]))
print(len(x[0]))

x = np.array(x)
print(x.shape)
