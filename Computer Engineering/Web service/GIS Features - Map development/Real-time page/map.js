import 'ol/ol.css';
import {Map, View} from 'ol/index';
import {Point, MultiPoint, MultiLineString} from 'ol/geom';
import TileLayer from 'ol/layer/Tile';
import {Vector as VectorLayer} from 'ol/layer';
import {OSM, Vector as VectorSource} from 'ol/source';
import {Circle, Fill, Style} from 'ol/style';
import {getVectorContext} from 'ol/render';
import {useGeographic} from 'ol/proj';
import {toPng} from 'html-to-image';
import {upAndDown} from 'ol/easing';
import GeoJSON from "ol/format/GeoJSON";
import Stroke from "ol/style/Stroke";
import {fromLonLat, toLonLat} from 'ol/proj';
import Overlay from 'ol/Overlay';

useGeographic();

var centerLat = JSON.parse(document.getElementById("center").innerText)[0]
var centerLon = JSON.parse(document.getElementById("center").innerText)[1]
var zoom = document.getElementById("zoom").innerText

var layer = new TileLayer({
    source: new OSM()
});

var layerLines = new TileLayer({
    source: new OSM()
});

var targetNodeLines = document.getElementById('geoJsonLines');

var sourceLines = new VectorSource({
    wrapX: false,
    features: new GeoJSON().readFeatures(JSON.parse(targetNodeLines.innerText))
});

var vectorLines = new VectorLayer({
    source: sourceLines,
});

var mapLines = new Map({
    layers: [layerLines, vectorLines],
    target: 'mapLines',
    view: new View({
        center: [centerLon,centerLat],
        zoom: zoom
    })
});

var styleFunction = function(feature) {
    if (feature.getGeometry().getType() == "LineString") {
        return new Style({
            stroke: new Stroke({
                color: feature.get('color'),
                width: mapLines.getView().getZoom() * 2 - 27
            })
        });
    }else{
        return new Style({
            image: new Circle({
                radius: 8,
                fill: new Fill({color: feature.get('color')})
            })
        })
    }
};

vectorLines.setStyle(styleFunction);

// Select the node that will be observed for mutations
var targetNode = document.getElementById('geoJsonPoints');

var sourcePoints = new VectorSource({
    wrapX: false,
    features: new GeoJSON().readFeatures(JSON.parse(targetNode.innerText))
});

var vectorPoints = new VectorLayer({
    source: sourcePoints,
});

var map = new Map({
    layers: [layer, vectorPoints],
    target: 'map',
    view: new View({
        center: [centerLon,centerLat],
        zoom: zoom
    })
});

vectorPoints.setStyle(styleFunction);

// Options for the observer (which mutations to observe)
var config = { childList: true, attributes: true };

// Callback function to execute when mutations are observed
var callback = function(mutationsList, observer) {
    for(var mutation of mutationsList) {
        if (mutation.type == 'childList' || mutation.type == 'attributes') {
            sourcePoints.refresh();
            sourcePoints.addFeatures(new GeoJSON().readFeatures(JSON.parse(targetNode.innerText)));
            sourceLines.refresh();
            sourceLines.addFeatures(new GeoJSON().readFeatures(JSON.parse(targetNodeLines.innerText)));
        }
    }
};

// Create an observer instance linked to the callback function
var observer = new MutationObserver(callback);

// Start observing the target node for configured mutations
observer.observe(targetNode, config);
observer.observe(targetNodeLines, config);

var tooltip = document.getElementById('tooltip');
var overlay = new Overlay({
    element: tooltip,
    offset: [10, 0],
    positioning: 'bottom-left'
});
map.addOverlay(overlay);

function displayTooltip(evt) {
    var pixel = evt.pixel;
    var feature = map.forEachFeatureAtPixel(pixel, function(feature) {
        return feature;
    });
    tooltip.style.display = feature ? '' : 'none';
    if (feature) {
        overlay.setPosition(evt.coordinate);
        tooltip.innerHTML = feature.get('name');
    }
};

map.on('pointermove', displayTooltip);

var tooltipLines = document.getElementById('tooltipLines');
var overlayLines = new Overlay({
    element: tooltipLines,
    offset: [10, 0],
    positioning: 'bottom-left'
});
mapLines.addOverlay(overlayLines);

function displayTooltipLines(evt) {
    var pixel = evt.pixel;
    var feature = mapLines.forEachFeatureAtPixel(pixel, function(feature) {
        return feature;
    });
    tooltipLines.style.display = feature ? '' : 'none';
    if (feature) {
        overlayLines.setPosition(evt.coordinate);
        tooltipLines.innerHTML = feature.get('name');
    }
};

mapLines.on('pointermove', displayTooltipLines);