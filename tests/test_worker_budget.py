import unittest
from builder.worker_budget import choose_workers,GIB
class BudgetContracts(unittest.TestCase):
 def test_larger_budget_requires_cpu_physical_and_available_headroom(self):
  self.assertEqual(choose_workers(18,64*GIB,20*GIB),4)
  self.assertEqual(choose_workers(8,16*GIB,8*GIB),4)
  self.assertEqual(choose_workers(8,16*GIB,7*GIB),2)
  self.assertEqual(choose_workers(8,8*GIB,6*GIB),2)
  self.assertEqual(choose_workers(4,8*GIB,3*GIB),1)
  self.assertEqual(choose_workers(2,64*GIB,20*GIB),1)
 def test_unknown_memory_falls_back_without_assuming_host_capability(self):
  self.assertEqual(choose_workers(18,None,None),1)
  self.assertEqual(choose_workers(18,64*GIB,None),1)
