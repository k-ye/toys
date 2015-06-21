import ConfigParser as cp
import inspect as ins

class RAIIObject:
  def __init__(self, obj):
    self._obj = obj
    for element in ins.getmembers(obj):
      name, method = element
      if '__' not in name:
        setattr(self, name, method)
    print 'RAII object %s is initialized' % self.name

  def __del__(self):
    self._obj.close()
    print 'RAII object %s is closed' % self.name

if __name__ == '__main__':
  rfp = open('input.ini', 'r')
  raii_rfp = RAIIObject(rfp)
  rparser = cp.ConfigParser()
  rparser.readfp(raii_rfp)

  print rparser.getint('Section1', 'attr1')
  print rparser.getboolean('Section1', 'attr2')
  print rparser.get('Section1', 'attr3')

  wfp = open('output.ini', 'w')
  raii_wfp = RAIIObject(wfp)
  wparser = cp.ConfigParser()
  wparser.add_section('Section3')
  wparser.set('Section3', 'attr1', 'Hellow World')
  wparser.set('Section3', 'attr2', 789)
  wparser.write(raii_wfp)