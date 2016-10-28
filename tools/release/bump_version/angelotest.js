var xpath = require('xpath');
var DOMParser = require('xmldom').DOMParser;

var doc = new DOMParser().parseFromString("<test><child1></child1></test>", 'text/xml');    

