(function(global) {

  function update(pTarget, pConfig) {
    pConfig.properties.targetId = pTarget.id;
    var tTargetName = pConfig.properties.target;

    if (!pConfig.isQuiet) print('Updating target ' + pTarget.id + ' (' + tTargetName + ')...');

    var tResourceHandlers = new Object();

    global.fire('queryResourceHandlers', tResourceHandlers);

    var tResources = pTarget.resources;

    var tGlobalResources = pConfig.resources;

    for (var i = 0, il = tResources.length; i < il; i++) {
      var tResource = tResources[i];
      var tResourceId;
      if (tResource.type === 'reference') {
        if (!tResource.name) {
          print('The name of the reference needs to be specified.');
          return;
        }
        if (!(tResource.name in tGlobalResources)) {
          print('The given resource "' + tResource.name + '" does not exist.');
          return;
        }
        tResourceId = tResource.name;
        var tNewResource = tGlobalResources[tResource.name];
        for (var k in tResource) {
          if (k !== 'type' && k !== 'name') {
            tNewResource[k] = tResource[k];
          }
        }
        tResource = tNewResource;
      } else {
        tResourceId = tTargetName + '__resource_' + i;
      }

      tResource.id = tResourceId;

      if (!(tResource.type in tResourceHandlers)) {
        print('The specified resource handler (' + tResource.type + ") is not supported.\nSupported types are:");
        for (var k in tResourceHandlers) {
          print('  ' + k);
        }
        return;
      }

      var tWorkspace = (pConfig.properties.buildDir || 'build') +
            '/' + tResourceId;

      if (global.stat(tWorkspace) === null) {
        system("mkdir -p '" + tWorkspace + "'");
      }

      var tResourceHandler = new tResourceHandlers[tResource.type](pConfig);
      tResourceHandler.setData(tResource, tWorkspace);
      if (!tResourceHandler.prepare()) {
        print('Failed to prepare resource. Aborting.');
        return;
      }
    }

    if (!pConfig.isQuiet) print('Finished updating target ' + pTarget.id + ' (' + tTargetName + ').');
  }

  function updateAction(pConfig, pTarget) {
    if (!pTarget) {
      if (typeof pConfig.raw.defaultTarget === 'string') {
        updateAction(pConfig, pConfig.raw.defaultTarget);
        return;
      }

      print('Please select a target. Valid targets are:');
      pConfig.printTargets();
    } else {
      var tTargets = pConfig.targets;
      var tGotOne = false;
      var tOutputs = new Array();
      for (var k in tTargets) {
        var tResult = tTargets[k].regex.exec(pTarget);
        if (tResult) {
          tGotOne = true;
          pConfig.properties.target = pTarget;
          for (var i = 1, il = tResult.length; i < il; i++) {
            pConfig.properties['target.' + i] = tResult[i];
          }
          update(tTargets[k], pConfig);
        }
      }
      if (tGotOne === false) {
        print('The target ' + pTarget + ' did not have any matches. Valid targets are:');
        pConfig.printTargets();
      }
    }
  }

  global.on('registerActions', function(pActions) {
    pActions.update = updateAction;
  });

}(this));