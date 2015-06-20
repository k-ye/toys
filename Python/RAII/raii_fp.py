import ConfigParser as cp

class RAIIFileObject:
		def __init__(self, fp):
				self._fp = fp
				# self.name = fp.name

		def __del__(self):
				self.close()
				print 'File object %s is closed.' % self._fp.name

		def read(self):
				return self._fp.read()

		def readline(self):
				return self._fp.readline()

		def readlines(self):
				return self._fp.readlines()

		def write(self, s):
				self._fp.write(s)

		def writelines(self, sequence):
				self._fp.writelines(sequence)

		def flush(self):
				self._fp.flush()

		def close(self):
				if not self._fp.closed:
					self._fp.close()

if __name__ == '__main__':
		rfp = open('input.ini', 'r')
		raii_rfp = RAIIFileObject(rfp)
		rparser = cp.ConfigParser()
		rparser.readfp(raii_rfp)

		print rparser.getint('Section1', 'attr1')
		print rparser.getboolean('Section1', 'attr2')
		print rparser.get('Section1', 'attr3')

		wfp = open('output.ini', 'w')
		raii_wfp = RAIIFileObject(wfp)
		wparser = cp.ConfigParser()
		wparser.add_section('Section3')
		wparser.set('Section3', 'attr1', 'Hellow World')
		wparser.set('Section3', 'attr2', 789)
		wparser.write(raii_wfp)


