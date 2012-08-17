/**                                                                                                                                    
 * @author Jason Parrott                                                                                                               
 *                                                                                                                                     
 * Copyright (C) 2012 Jason Parrott.                                                                                                   
 * This code is licensed under the zlib license. See LICENSE for details.                                                              
 */


#include <iostream>
#include <map>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <v8.h>
#include "api.h"

using namespace std;
using namespace v8;


Persistent<Context> sCreateContext() {
  Handle<ObjectTemplate> tGlobal = ObjectTemplate::New();

  tGlobal->Set(String::New("print"), FunctionTemplate::New(sPrint));

  tGlobal->Set(String::New("input"), FunctionTemplate::New(sInput));

  tGlobal->Set(String::New("read"), FunctionTemplate::New(sRead));

  tGlobal->Set(String::New("write"), FunctionTemplate::New(sWrite));

  tGlobal->Set(String::New("fork"), FunctionTemplate::New(sFork));

  tGlobal->Set(String::New("sleep"), FunctionTemplate::New(sSleep));

  tGlobal->Set(String::New("system"), FunctionTemplate::New(sSystem));

  tGlobal->Set(String::New("chdir"), FunctionTemplate::New(sChdir));

  tGlobal->Set(String::New("getcwd"), FunctionTemplate::New(sGetcwd));
  
  tGlobal->Set(String::New("evalFileInSandbox"), FunctionTemplate::New(sEvalFileInSandbox));

  tGlobal->Set(String::New("evalInSandbox"), FunctionTemplate::New(sEvalInSandbox));

  tGlobal->Set(String::New("evalFile"), FunctionTemplate::New(sEvalFile));

  return Context::New(NULL, tGlobal);
}

Handle<Value> sPrint(const Arguments &pArgs) {
  bool tFirst = true;
  for (int i = 0, il = pArgs.Length(); i < il; i++) {
    if (tFirst == true) {
      tFirst = false;
    } else {
      printf(" ");
    }

    String::Utf8Value tStr(pArgs[i]);
    const char* tCStr = sToCString(tStr);

    printf("%s", tCStr);
  }
  printf("\n");
  fflush(stdout);
  return Undefined();
}

Handle<Value> sInput(const Arguments &pArgs) {
  string tIn;
  string tFinal = "";
  while (cin) {
    getline(cin, tIn);
    tFinal += tIn;
  }

  Handle<String> tResult = String::New(tFinal.c_str());
  return tResult;
}

Handle<Value> sRead(const Arguments &pArgs) {
  if (pArgs.Length() != 1) {
    return ThrowException(String::New("Too many arguments."));
  }
  
  String::Utf8Value tStr(pArgs[0]);
  return sReadFile(sToCString(tStr));
}

Handle<Value> sWrite(const Arguments &pArgs) {
  if (pArgs.Length() != 2) {
    return ThrowException(String::New("Too many arguments."));
  }

  String::Utf8Value tFileName(pArgs[0]);
  String::Utf8Value tContent(pArgs[1]);

  return sWriteFile(sToCString(tFileName), sToCString(tContent), tContent.length());
}

Handle<Value> sFork(const Arguments &pArgs) {
  if (pArgs.Length() != 0) {
    return ThrowException(String::New("Too many arguments."));
  }
  
  pid_t tPid = fork();
  return Integer::NewFromUnsigned(tPid);
}

Handle<Value> sSleep(const Arguments &pArgs) {
  if (pArgs.Length() != 1) {
    return ThrowException(String::New("Too many arguments"));
  }
  Local<Integer> tTime = Local<Integer>::Cast(pArgs[0]);
  sleep(tTime->Int32Value());
  return Undefined();
}

Handle<Value> sSystem(const Arguments &pArgs) {
  if (pArgs.Length() != 1) {
    return ThrowException(String::New("Too many arguments"));
  }

  String::Utf8Value tCommand(pArgs[0]);
  FILE *tFp = popen(*tCommand, "r");

  if (tFp == NULL) {
    return ThrowException(String::New("Failed to execute command."));
  }

  char tBuffer[256];
  string tResult = "";

  while (fgets(tBuffer, 256, tFp) != NULL) {
    tResult += tBuffer;
  }

  pclose(tFp);

  Handle<String> tValue = String::New(tResult.c_str(), tResult.length());

  return tValue;
}

Handle<Value> sChdir(const Arguments &pArgs) {
  if (pArgs.Length() != 1) {
    return ThrowException(String::New("Too many arguments"));
  }

  String::Utf8Value tDirectory(pArgs[0]);
  if (chdir(*tDirectory) != 0) {
    return ThrowException(String::New("Failed to change to given directory."));
  }

  return Undefined();
}

Handle<Value> sGetcwd(const Arguments &pArgs) {
  // TODO: Support any size of directory name...
  char *dir = (char *)malloc(sizeof(char) * 1024);
  if (dir == NULL) {
    return ThrowException(String::New("Out of memory"));
  }
  
  if (getcwd(dir, sizeof(char) * 1024) == NULL) {
    return ThrowException(String::New("Could not get current working directory"));
  }

  Handle<String> tReturn = String::New(dir);

  free(dir);

  return tReturn;
}

inline void cleanContextIfSandboxed(Handle<Context> pContext, bool pIsSandboxed) {
  if (pIsSandboxed) {
    Persistent<Context> tContext = (Persistent<Context>)pContext;
    tContext->Exit();
    tContext.Dispose();
  }
}

Handle<Value> sEvalScript(const Arguments &pArgs, bool pIsSandboxed, bool pIsString) {
  if (!pArgs[0]->IsString()) {
    return ThrowException(String::New("First argument must be a string"));
  }

  if (pIsSandboxed && !pArgs[1]->IsObject()) {
    return ThrowException(String::New("Second argument must be an object or null"));
  }

  Handle<Value> tContentsAsValue;

  if (!pIsString) {
    String::Utf8Value tFileName(pArgs[0]);
    tContentsAsValue = sReadFile(*tFileName);
  } else {
    tContentsAsValue = pArgs[0];
  }

  if (tContentsAsValue.IsEmpty()) {
    if (pIsString) {
      return ThrowException(String::New("Invalid string to eval"));
    } else {
      return ThrowException(String::New("Failed to open file"));
    }
  }

  HandleScope tScope;
  Handle<Context> tContext = Context::GetCurrent();

  if (pIsSandboxed) {
    Handle<Object> tGlobalObject = Handle<Object>::Cast(pArgs[1]);

    Handle<Array> tProperties = tGlobalObject->GetPropertyNames();
    Handle<ObjectTemplate> tGlobal = ObjectTemplate::New();

    for (int i = 0, il = tProperties->Length(); i < il; i++) {
      Handle<String> tKey = Handle<String>::Cast(tProperties->Get(i));
      Handle<Value> tValue = tGlobalObject->Get(tKey);
      tGlobal->Set(tKey, tValue);
    }
    
    tContext = Context::New(NULL, tGlobal);
    tContext->Enter();
  }

  TryCatch tTryCatch;
  
  String::Utf8Value tScriptToEval(tContentsAsValue);
  Handle<String> tScriptHandle = String::New(*tScriptToEval);

  Handle<Script> tScript = Script::Compile(tScriptHandle);

  if (tScript.IsEmpty()) {
    cleanContextIfSandboxed(tContext, pIsSandboxed);
    return ThrowException(tTryCatch.Exception());
  }

  Handle<Value> tResult = tScript->Run();

  if (tResult.IsEmpty()) {
    cleanContextIfSandboxed(tContext, pIsSandboxed);
    return ThrowException(tTryCatch.Exception());
  }

  cleanContextIfSandboxed(tContext, pIsSandboxed);

  return Undefined();
}

Handle<Value> sEvalFileInSandbox(const Arguments &pArgs) {
  return sEvalScript(pArgs, true, false);
}

Handle<Value> sEvalInSandbox(const Arguments &pArgs) {
  return sEvalScript(pArgs, true, true);
}

Handle<Value> sEvalFile(const Arguments &pArgs) {
  return sEvalScript(pArgs, false, false);
}

Handle<Value> sReadFile(const char *pName) {
  FILE *tFile = fopen(pName, "rb");
  if (tFile == NULL) {
    return ThrowException(String::New("File doesn't exist."));
  }

  fseek(tFile, 0, SEEK_END);
  int tSize = ftell(tFile);
  rewind(tFile);

  char *tChars = new char[tSize + 1];
  tChars[tSize] = '\n';

  for (int i = 0; i < tSize;) {
    int tRead = fread(&tChars[i], 1, tSize - i, tFile);
    i += tRead;
  }

  fclose(tFile);

  Handle<String> tResult = String::New(tChars, tSize);
  delete[] tChars;

  return tResult;
}

Handle<Value> sWriteFile(const char *pName, const char *pContent, int pLength) {
  FILE *tFile = fopen(pName, "wb");
  if (tFile == NULL) {
    return ThrowException(String::New("Could not open file for writing."));
  }

  if (fwrite(pContent, 1, pLength, tFile) != pLength) {
    fclose(tFile);
    return ThrowException(String::New("Failed writing all the data to file."));
  }

  fclose(tFile);

  return Undefined();
}

void sReportException(TryCatch *pHandler) {
  printf("Exception occurred\n");
}