from pynq import Xlnk
import numpy as np

xlnk=Xlnk()

input_buffer = xlnk.cma_array(shape=1024 , dtype=int)
output_buffer = xlnk.cma_array(shape=1024 , dtype=int)
xlnk.cma_stats()
t = xlnk.cma_get_phy_addr(input_buffer.pointer)
v = xlnk.cma_get_phy_addr(output_buffer.pointer)
output_buffer.freebuffer()
input_buffer.freebuffer()

