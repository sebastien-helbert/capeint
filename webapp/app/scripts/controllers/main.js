'use strict';

/**
 * @ngdoc function
 * @name guirlandeApp.controller:MainCtrl
 * @description
 * # MainCtrl
 * Controller of the guirlandeApp
 */
angular.module('guirlandeApp')
  .controller('MainCtrl', ['$scope', '$log', '$location', function ($scope, $log, $location) {

    var client;

    $scope.options = {
      format: 'rgb', alpha: false
    };
    $scope.color1 = {r:255, g:255, b:255};
    $scope.speed = 0;
    $scope.brightness = 0;


    $scope.mqtt = {
      port: 9001,
      url: "ws://" + $location.host(),
      connected: false,
      connecting: false,
      everConnected: false
    }

    function reconnect() {
      disconnect();
      connect($scope.mqtt.url, $scope.mqtt.port);
    }

    function connect(host, port) {
      var url = host + ':' + Number(port) + '/';
      $log.debug("Trying to connect to " + url);
      client = new Paho.MQTT.Client(url, getClientId());
      client.onConnectionLost = onConnectionLost;
      client.onMessageArrived = onMessageArrived;
      client.connect({
        onSuccess: onConnect, // after connected, subscribes
        onFailure: function () {
          console.error(arguments)
        }     // useful for logging / debugging
      });
      $scope.mqtt.connecting = true;
    }

    function getClientId() {
      var id;
      try {
        id = localStorage['mqtt-id']
      } catch (e) {
      }
      if (!id) {
        id = 'guirlande_' + (Math.random() * 100000).toFixed(0);
      }
      return id;
    }

    function sendMessage(topic, msg, qos) {
      var message = new Paho.MQTT.Message(String(msg));
      message.destinationName = topic;
      message.qos = qos || 0;
      $log.debug('Sending message to topic [' + topic + '] : ' + msg);
      client.send(message);
    }

    function disconnect() {
      if (client) {
        client.disconnect();
      }
      $scope.mqtt.connected = false;
      $scope.mqtt.connecting = false;
    }

    function onConnect(context) {
      console.info("Client Connected");
      $scope.mqtt.connecting = false;
      $scope.mqtt.connected = true;
      $scope.mqtt.everConnected = true;
      // And subscribe to our topics
      client.subscribe('color1');
      client.subscribe('color2');
      client.subscribe('color3');
      client.subscribe('speed');
      client.subscribe('brightness');

      sendMessage('feedback', 'what\'s up');
      $scope.$apply();
    }


    function onConnectionLost(responseObject) {
      if (responseObject.errorCode !== 0) {
        $log.debug("Connection Lost: " + responseObject.errorMessage);
      }
      $scope.$apply();
    }

    function onMessageArrived(message) {
      $log.debug('New message received from topic [' + message.destinationName + '] : ' + message.payloadString);

      if (message.destinationName === 'color1') {
        var splittedPayload = message.payloadString.split(',');
        $scope.color1 = {r : splittedPayload[0], g : splittedPayload[1], b : splittedPayload[2]};
      } else if (message.destinationName === 'color2') {
        var splittedPayload = message.payloadString.split(',');
        $scope.color2 = {r : splittedPayload[0], g : splittedPayload[1], b : splittedPayload[2]};
      } else if (message.destinationName === 'color3') {
        var splittedPayload = message.payloadString.split(',');
        $scope.color3 = {r : splittedPayload[0], g : splittedPayload[1], b : splittedPayload[2]};
      } else if (message.destinationName === 'speed') {
        $scope.speed = parseInt(message.payloadString, 10);
      } else if (message.destinationName === 'brightness') {
        $scope.brightness = parseInt(message.payloadString, 10);
      } else {
        $log.debug('unknown topic [' + message.destinationName + '] for message [' + message.payloadString + '].');
      }
      $scope.$apply();
    }

    reconnect();

    $scope.changeColor = function (color, topic) {
      var alpha = 0;
      var red = parseInt(color.r);
      var green = parseInt(color.g);
      var blue = parseInt(color.b);
      sendMessage(topic, red + ',' + green + ',' + blue + ',' + alpha);
    };

    $scope.updateSpeed = function (slider) {
      sendMessage('speed', slider.from);
    };

    $scope.updateBrightness = function (slider) {
     sendMessage('brightness', slider.from);
    }

}]);
