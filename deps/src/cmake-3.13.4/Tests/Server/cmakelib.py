from __future__ import print_function
import sys, subprocess, json, os, select, shutil, time, socket

termwidth = 150

print_communication = True

def ordered(obj):
  if isinstance(obj, dict):
    return sorted((k, ordered(v)) for k, v in obj.items())
  if isinstance(obj, list):
    return sorted(ordered(x) for x in obj)
  else:
    return obj

def col_print(title, array):
  print()
  print()
  print(title)

  indentwidth = 4
  indent = " " * indentwidth

  if not array:
    print(indent + "<None>")
    return

  padwidth = 2

  maxitemwidth = len(max(array, key=len))

  numCols = max(1, int((termwidth - indentwidth + padwidth) / (maxitemwidth + padwidth)))

  numRows = len(array) // numCols + 1

  pad = " " * padwidth

  for index in range(numRows):
    print(indent + pad.join(item.ljust(maxitemwidth) for item in array[index::numRows]))

filterPacket = lambda x: x

STDIN = 0
PIPE = 1

communicationMethods = [STDIN]

if hasattr(socket, 'AF_UNIX'):
  communicationMethods.append(PIPE)

def defaultExitWithError(proc):
  data = ""
  try:
    while select.select([proc.outPipe], [], [], 3.)[0]:
      data = data + proc.outPipe.read(1)
    if len(data):
      print("Rest of raw buffer from server:")
      printServer(data)
  except:
    pass
  proc.outPipe.close()
  proc.inPipe.close()
  proc.kill()
  sys.exit(1)

exitWithError = lambda proc: defaultExitWithError(proc)

serverTag = "SERVER"

def printServer(*args):
    print(serverTag + ">", *args)
    print()
    sys.stdout.flush()

def printClient(*args):
    print("CLIENT>", *args)
    print()
    sys.stdout.flush()

def waitForRawMessage(cmakeCommand):
  stdoutdata = ""
  payload = ""
  while not cmakeCommand.poll():
    stdoutdataLine = cmakeCommand.outPipe.readline()
    if stdoutdataLine:
      stdoutdata += stdoutdataLine.decode('utf-8')
    else:
      break
    begin = stdoutdata.find('[== "CMake Server" ==[\n')
    end = stdoutdata.find(']== "CMake Server" ==]')

    if begin != -1 and end != -1:
      begin += len('[== "CMake Server" ==[\n')
      payload = stdoutdata[begin:end]
      jsonPayload = json.loads(payload)
      filteredPayload = filterPacket(jsonPayload)
      if print_communication and filteredPayload:
        printServer(filteredPayload)
      if filteredPayload is not None or jsonPayload is None:
          return jsonPayload
      stdoutdata = stdoutdata[(end+len(']== "CMake Server" ==]')):]

# Python2 has no problem writing the output of encodes directly,
# but Python3 returns only 'int's for encode and so must be turned
# into bytes. We use the existence of 'to_bytes' on an int to
# determine which behavior is appropriate. It might be more clear
# to do this in the code which uses the flag, but introducing
# this lookup cost at every byte sent isn't ideal.
has_to_bytes = "to_bytes" in dir(10)

def writeRawData(cmakeCommand, content):
  writeRawData.counter += 1
  payload = """
[== "CMake Server" ==[
%s
]== "CMake Server" ==]
""" % content

  rn = ( writeRawData.counter % 2 ) == 0

  if rn:
    payload = payload.replace('\n', '\r\n')

  if print_communication:
    printClient(content, "(Use \\r\\n:", rn, ")")

  # To stress test how cmake deals with fragmentation in the
  # communication channel, we send only one byte at a time.
  # Certain communication methods / platforms might still buffer
  # it all into one message since its so close together, but in
  # general this will catch places where we assume full buffers
  # come in all at once.
  encoded_payload = payload.encode('utf-8')

  # Python version 3+ can't write ints directly; but 'to_bytes'
  # for int was only added in python 3.2. If this is a 3+ version
  # of python without that conversion function; just write the whole
  # thing out at once.
  if sys.version_info[0] > 2 and not has_to_bytes:
    cmakeCommand.write(encoded_payload)
  else:
    for c in encoded_payload:
      if has_to_bytes:
        c = c.to_bytes(1, byteorder='big')
      cmakeCommand.write(c)

writeRawData.counter = 0

def writePayload(cmakeCommand, obj):
  writeRawData(cmakeCommand, json.dumps(obj))

def getPipeName():
  return "/tmp/server-test-socket"

def attachPipe(cmakeCommand, pipeName):
  time.sleep(1)
  sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
  sock.connect(pipeName)
  global serverTag
  serverTag = "SERVER(PIPE)"
  cmakeCommand.outPipe = sock.makefile()
  cmakeCommand.inPipe = sock
  cmakeCommand.write = cmakeCommand.inPipe.sendall

def writeAndFlush(pipe, val):
  pipe.write(val)
  pipe.flush()

def initServerProc(cmakeCommand, comm):
  if comm == PIPE:
    pipeName = getPipeName()
    cmakeCommand = subprocess.Popen([cmakeCommand, "-E", "server", "--experimental", "--pipe=" + pipeName])
    attachPipe(cmakeCommand, pipeName)
  else:
    cmakeCommand = subprocess.Popen([cmakeCommand, "-E", "server", "--experimental", "--debug"],
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE)
    cmakeCommand.outPipe = cmakeCommand.stdout
    cmakeCommand.inPipe = cmakeCommand.stdin
    cmakeCommand.write = lambda val: writeAndFlush(cmakeCommand.inPipe, val)

  packet = waitForRawMessage(cmakeCommand)
  if packet == None:
    print("Not in server mode")
    sys.exit(2)

  if packet['type'] != 'hello':
    print("No hello message")
    sys.exit(3)

  return cmakeCommand

def exitProc(cmakeCommand):
  # Tell the server to exit.
  cmakeCommand.stdin.close()
  cmakeCommand.stdout.close()

  # Wait for the server to exit.
  # If this version of python supports it, terminate the server after a timeout.
  try:
    cmakeCommand.wait(timeout=5)
  except TypeError:
    cmakeCommand.wait()
  except:
    cmakeCommand.terminate()
    raise

def waitForMessage(cmakeCommand, expected):
  data = ordered(expected)
  packet = ordered(waitForRawMessage(cmakeCommand))

  if packet != data:
    print ("Received unexpected message; test failed")
    exitWithError(cmakeCommand)
  return packet

def waitForReply(cmakeCommand, originalType, cookie, skipProgress):
  gotResult = False
  while True:
    packet = waitForRawMessage(cmakeCommand)
    t = packet['type']
    if packet['cookie'] != cookie or packet['inReplyTo'] != originalType:
      print("cookie or inReplyTo mismatch")
      sys.exit(4)
    if t == 'message' or t == 'progress':
      if skipProgress:
        continue
    if t == 'reply':
        break
    print("Unrecognized message", packet)
    sys.exit(5)

  return packet

def waitForError(cmakeCommand, originalType, cookie, message):
  packet = waitForRawMessage(cmakeCommand)
  if packet['cookie'] != cookie or packet['type'] != 'error' or packet['inReplyTo'] != originalType or packet['errorMessage'] != message:
    sys.exit(6)

def waitForProgress(cmakeCommand, originalType, cookie, current, message):
  packet = waitForRawMessage(cmakeCommand)
  if packet['cookie'] != cookie or packet['type'] != 'progress' or packet['inReplyTo'] != originalType or packet['progressCurrent'] != current or packet['progressMessage'] != message:
    sys.exit(7)

def handshake(cmakeCommand, major, minor, source, build, generator, extraGenerator):
  version = { 'major': major }
  if minor >= 0:
    version['minor'] = minor

  writePayload(cmakeCommand, { 'type': 'handshake', 'protocolVersion': version,
    'cookie': 'TEST_HANDSHAKE', 'sourceDirectory': source, 'buildDirectory': build,
    'generator': generator, 'extraGenerator': extraGenerator })
  waitForReply(cmakeCommand, 'handshake', 'TEST_HANDSHAKE', False)

def validateGlobalSettings(cmakeCommand, cmakeCommandPath, data):
  packet = waitForReply(cmakeCommand, 'globalSettings', '', False)

  capabilities = packet['capabilities']

  # validate version:
  cmakeoutput = subprocess.check_output([ cmakeCommandPath, "--version" ], universal_newlines=True)
  cmakeVersion = cmakeoutput.splitlines()[0][14:]

  version = capabilities['version']
  versionString = version['string']
  vs = str(version['major']) + '.' + str(version['minor']) + '.' + str(version['patch'])
  if (versionString != vs and not versionString.startswith(vs + '-')):
    sys.exit(8)
  if (versionString != cmakeVersion):
    sys.exit(9)

  # validate generators:
  generatorObjects = capabilities['generators']

  cmakeoutput = subprocess.check_output([ cmakeCommandPath, "--help" ], universal_newlines=True)
  index = cmakeoutput.index('\nGenerators\n\n')
  cmakeGenerators = []
  for line in cmakeoutput[index + 12:].splitlines():
    if not line.startswith('  '):
      continue
    if line.startswith('    '):
      continue
    equalPos = line.find('=')
    tmp = ''
    if (equalPos > 0):
      tmp = line[2:equalPos].strip()
    else:
      tmp = line.strip()
    if tmp.endswith(" [arch]"):
      tmp = tmp[0:len(tmp) - 7]
    if (len(tmp) > 0) and (" - " not in tmp):
      cmakeGenerators.append(tmp)

  generators = []
  for genObj in generatorObjects:
    generators.append(genObj['name'])

  generators.sort()
  cmakeGenerators.sort()

  for gen in cmakeGenerators:
    if (not gen in generators):
        sys.exit(10)

  gen = packet['generator']
  if (gen != '' and not (gen in generators)):
    sys.exit(11)

  for i in data:
    print("Validating", i)
    if (packet[i] != data[i]):
      sys.exit(12)

def validateCache(cmakeCommand, data):
  packet = waitForReply(cmakeCommand, 'cache', '', False)

  cache = packet['cache']

  if (data['isEmpty']):
    if (cache != []):
      print('Expected empty cache, but got data.\n')
      sys.exit(1)
    return;

  if (cache == []):
    print('Expected cache contents, but got none.\n')
    sys.exit(1)

  hadHomeDir = False
  for value in cache:
    if (value['key'] == 'CMAKE_HOME_DIRECTORY'):
      hadHomeDir = True

  if (not hadHomeDir):
    print('No CMAKE_HOME_DIRECTORY found in cache.')
    sys.exit(1)

def handleBasicMessage(proc, obj, debug):
  if 'sendRaw' in obj:
    data = obj['sendRaw']
    if debug: print("Sending raw:", data)
    writeRawData(proc, data)
    return True
  elif 'send' in obj:
    data = obj['send']
    if debug: print("Sending:", json.dumps(data))
    writePayload(proc, data)
    return True
  elif 'recv' in obj:
    data = obj['recv']
    if debug: print("Waiting for:", json.dumps(data))
    waitForMessage(proc, data)
    return True
  elif 'message' in obj:
    print("MESSAGE:", obj["message"])
    sys.stdout.flush()
    return True
  return False

def shutdownProc(proc):
  # Tell the server to exit.
  proc.inPipe.close()
  proc.outPipe.close()

  # Wait for the server to exit.
  # If this version of python supports it, terminate the server after a timeout.
  try:
    proc.wait(timeout=5)
  except TypeError:
    proc.wait()
  except:
    proc.terminate()
    raise

  print('cmake-server exited: %d' % proc.returncode)
  sys.exit(proc.returncode)
