'use strict';

describe('Directive: materialPicker', function () {

  // load the directive's module
  beforeEach(module('guirlandeApp'));

  var element,
    scope;

  beforeEach(inject(function ($rootScope) {
    scope = $rootScope.$new();
  }));

  it('should make hidden element visible', inject(function ($compile) {
    element = angular.element('<material-picker></material-picker>');
    element = $compile(element)(scope);
    expect(element.text()).toBe('this is the materialPicker directive');
  }));
});
