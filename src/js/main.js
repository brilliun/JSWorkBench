/**                                                                                                                                    
 * @author Jason Parrott                                                                                                               
 *                                                                                                                                     
 * Copyright (C) 2012 Jason Parrott.                                                                                                   
 * This code is licensed under the zlib license. See LICENSE for details.                                                              
 */


(function(global) {
  var print = global.print,
      read = global.read,
      fire = global.fire,
      args = global.args;

  function getVersion() {
    return '1.0';
  }
  global.getVersion = getVersion;

  function printVersion() {
    print(getVersion());
  }
  global.printVersion = printVersion;

  function main() {
    var tWorkBench = new WorkBench();

    function printUsage() {
      print("Usage: jsworkbench [-f BUILD_FILE] [--dry] ACTION [arg1[, arg2...]");
      print("Actions are:");
      for (var k in tWorkBench.actions) {
        print(' ' + k);
      }
    }

    tWorkBench.actions.help = printUsage;

    if (!tWorkBench.load()) {
      printUsage();
      return 1;
    }

    if (args.length < 2) {
      printUsage();
      return 2;
    }

    var i = 1;
    for (var il = args.length; i < il; i++) {
      if (args[i][0] !== '-') break;
    }
    var tActionName = args[i];

    if (tWorkBench.runAction(tActionName, args.slice(i + 1)) === false) {
      printUsage();
      return 3;
    }
  }
  global.main = main;

}(this));

