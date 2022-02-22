var project = new Project('pbrcpp');

project.addFile('Sources/**');
project.setDebugDir('Deployment');
project.targetOptions.android.package = 'com.gitindro.render';
// https://developer.android.com/guide/topics/manifest/activity-element.html#screen
project.targetOptions.android.screenOrientation = 'portrait';
project.targetOptions.android.permissions = ['android.permission.INTERNET'];
// https://developer.android.com/guide/topics/manifest/manifest-element#install
project.targetOptions.android.installLocation = "auto";

resolve(project);