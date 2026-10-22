import unittest

from check_handle_cache import inspect


class CacheProbeTest(unittest.TestCase):
    def test_rejects_duplicate_bdf(self):
        values = {
            "nv_pci_handles::walk list": "0x1000\n0x2000\n",
            "0x1000/5K": "0x1000: 2000 0 ffff 1234 10de000000010100\n",
            "0x2000/5K": "0x2000: 0 0 0 0 10de000000010100\n",
        }
        with self.assertRaisesRegex(ValueError, "duplicate BDF"):
            inspect(values.__getitem__, 0x100)

    def test_requires_attached_gpu_fields(self):
        values = {
            "nv_pci_handles::walk list": "0x1000\n",
            "0x1000/5K": "0x1000: 0 0 0 0 10de000000010100\n",
        }
        with self.assertRaisesRegex(ValueError, "lacks dip/cfg"):
            inspect(values.__getitem__, 0x100)


if __name__ == "__main__":
    unittest.main()
