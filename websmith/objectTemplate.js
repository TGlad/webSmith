// < This code is user generated, don't adjust manually >
[objectName]Count = 0;
[objectName] = function()
{
  this.outDest = {};
  this.backInSource = {};
  this.input = {};
  this.topState = {};
  this.down = {};
  this.out = {};
  this.backIn = {};
  this.baseState = {};
  this.up = {};
  this.backOut = {};
  this.inSource = {};
  this.backOutDest = {};
  
  this.name = '[objectName]';
  [objectName]Count++;
  if ([objectName]Count > 1)
    this.name += [objectName]Count;
}
[objectName].prototype.init = function(input, topState, down, out, backIn, baseState, up, backOut, outDest, backInSource, inSource, backOutDest, parameters)
{
  [init]
}
[objectName].prototype.receive = function(input, topState, down)
{
  [receive]
}
[objectName].prototype.reply = function(up, backOut, topState)
{
  [reply]
}
[objectName].prototype.predict = function(down, topState, up, timeStep)
{
  [predict]
}
[objectName].prototype.react = function(up, baseState, down, timeStep)
{
  [react]
}
[objectName].prototype.generate = function(down, out, baseState)
{
  [generate]
}
[objectName].prototype.analyse = function(backIn, baseState, up)
{
  [analyse]
}
// <end>