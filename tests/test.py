import cmake_example as m
import os
import sys
import numpy as np
import time

bs = sys.argv[1]
filepath = sys.argv[2]
os.environ["BATCH_SIZE"] = bs
os.environ["MP4_PATH"] = filepath

bs = int(bs)
x = m.add(1, 2)
files = [filepath] * bs
indices = [np.array([32, 64, 128, 256, 512, 1024, 2048, 4096]).tolist()] * bs
start = time.time()
x = m.multi_thread_loader(files, indices)
end = time.time()

print(f"time is {end - start} seconds")
print(type(x))
print(len(x))
print(type(x[0]))
print(len(x[0]))

x = np.array(x)
print(x.shape)
