from __future__ import print_function
import sys, cmakelib, json, os, shutil

debug = True

cmakeCommand = sys.argv[1]
testFile = sys.argv[2]
sourceDir = sys.argv[3]
buildDir = sys.argv[4] + "/" + os.path.splitext(os.path.basename(testFile))[0]
cmakeGenerator = sys.argv[5]

print("Server Test:", testFile,
      "\n-- SourceDir:", sourceDir,
      "\n-- BuildDir:", buildDir,
      "\n-- Generator:", cmakeGenerator)

if os.path.exists(buildDir):
    shutil.rmtree(buildDir)

cmakelib.filterBase = sourceDir

with open(testFile) as f:
    testData = json.loads(f.read())

for communicationMethod in cmakelib.communicationMethods:
    proc = cmakelib.initServerProc(cmakeCommand, communicationMethod)
    if proc is None:
        continue

    for obj in testData:
        if cmakelib.handleBasicMessage(proc, obj, debug):
            pass
        elif 'reply' in obj:
            data = obj['reply']
            if debug: print("Waiting for reply:", json.dumps(data))
            originalType = ""
            cookie = ""
            skipProgress = False;
            if 'cookie' in data: cookie = data['cookie']
            if 'type' in data: originalType = data['type']
            if 'skipProgress' in data: skipProgress = data['skipProgress']
            cmakelib.waitForReply(proc, originalType, cookie, skipProgress)
        elif 'error' in obj:
            data = obj['error']
            if debug: print("Waiting for error:", json.dumps(data))
            originalType = ""
            cookie = ""
            message = ""
            if 'cookie' in data: cookie = data['cookie']
            if 'type' in data: originalType = data['type']
            if 'message' in data: message = data['message']
            cmakelib.waitForError(proc, originalType, cookie, message)
        elif 'progress' in obj:
            data = obj['progress']
            if debug: print("Waiting for progress:", json.dumps(data))
            originalType = ''
            cookie = ""
            current = 0
            message = ""
            if 'cookie' in data: cookie = data['cookie']
            if 'type' in data: originalType = data['type']
            if 'current' in data: current = data['current']
            if 'message' in data: message = data['message']
            cmakelib.waitForProgress(proc, originalType, cookie, current, message)
        elif 'handshake' in obj:
            data = obj['handshake']
            if debug: print("Doing handshake:", json.dumps(data))
            major = -1
            minor = -1
            generator = cmakeGenerator
            extraGenerator = ''
            sourceDirectory = sourceDir
            buildDirectory = buildDir
            if 'major' in data: major = data['major']
            if 'minor' in data: minor = data['minor']
            if 'buildDirectory' in data: buildDirectory = data['buildDirectory']
            if 'sourceDirectory' in data: sourceDirectory = data['sourceDirectory']
            if 'generator' in data: generator = data['generator']
            if 'extraGenerator' in data: extraGenerator = data['extraGenerator']

            if not os.path.isabs(buildDirectory):
                buildDirectory = buildDir + "/" + buildDirectory
            if sourceDirectory != '' and not os.path.isabs(sourceDirectory):
                sourceDirectory = sourceDir + "/" + sourceDirectory
            cmakelib.handshake(proc, major, minor, sourceDirectory, buildDirectory,
                               generator, extraGenerator)
        elif 'validateGlobalSettings' in obj:
            data = obj['validateGlobalSettings']
            if not 'buildDirectory' in data: data['buildDirectory'] = buildDir
            if not 'sourceDirectory' in data: data['sourceDirectory'] = sourceDir
            if not 'generator' in data: data['generator'] = cmakeGenerator
            if not 'extraGenerator' in data: data['extraGenerator'] = ''
            cmakelib.validateGlobalSettings(proc, cmakeCommand, data)
        elif 'validateCache' in obj:
            data = obj['validateCache']
            if not 'isEmpty' in data: data['isEmpty'] = false
            cmakelib.validateCache(proc, data)
        elif 'reconnect' in obj:
            cmakelib.exitProc(proc)
            proc = cmakelib.initServerProc(cmakeCommand, communicationMethod)
        else:
            print("Unknown command:", json.dumps(obj))
            sys.exit(2)
    cmakelib.shutdownProc(proc)
    print("Completed")
