import 'ol/ol.css';
import {Map, View} from 'ol/index';
import TileLayer from 'ol/layer/Tile';
import {Vector as VectorLayer} from 'ol/layer';
import {OSM, Vector as VectorSource} from 'ol/source';
import {Circle, Fill, Icon, Style} from 'ol/style';
import {useGeographic} from 'ol/proj';
import {toPng} from 'html-to-image';
import GeoJSON from "ol/format/GeoJSON";
import Stroke from "ol/style/Stroke";
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

var layerIcons = new TileLayer({
    source: new OSM()
});

var targetNodeLines = document.getElementById('geoJson');

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

// Select the node that will be observed for mutations
var targetNodeIcons = document.getElementById('geoJsonIcons');

var sourceIcons = new VectorSource({
    wrapX: false,
    features: new GeoJSON().readFeatures(JSON.parse(targetNodeIcons.innerText))
});

var vectorIcons = new VectorLayer({
    source: sourceIcons,
});

var mapIcons = new Map({
    layers: [layerIcons, vectorIcons],
    target: 'mapIcons',
    view: new View({
        center: [centerLon,centerLat],
        zoom: zoom
    })
});

var image_dict = {
    "air_conditioner": document.getElementById("airConditionerImage").src,
    "car_horn": document.getElementById("carHornImage").src,
    "children_playing": document.getElementById("childrenPlayingImage").src,
    "dog_bark": document.getElementById("dogBarkImage").src,
    "drilling": document.getElementById("drillingImage").src,
    "engine_idling": document.getElementById("engineIdlingImage").src,
    "gun_shot": document.getElementById("gunShotImage").src,
    "jackhammer": document.getElementById("jackhammerImage").src,
    "siren": document.getElementById("sirenImage").src,
    "street_music": document.getElementById("streetMusicImage").src
};

var styleFunctionIcons = function(feature) {
    return new Style({
        image: new Icon({
            src: image_dict[feature.get('class')],
            scale:0.05
        })
    })
};

vectorIcons.setStyle(styleFunctionIcons);


// Options for the observer (which mutations to observe)
var config = { childList: true, attributes: true };

// Callback function to execute when mutations are observed
var callback = function(mutationsList, observer) {
    for(var mutation of mutationsList) {
        if (mutation.type == 'childList' || mutation.type == 'attributes') {
            map.getView().setCenter([JSON.parse(document.getElementById("center").innerText)[1], JSON.parse(document.getElementById("center").innerText)[0]]);
            map.getView().setZoom(document.getElementById("zoom").innerText)
            sourcePoints.refresh();
            sourcePoints.addFeatures(new GeoJSON().readFeatures(JSON.parse(targetNode.innerText)));
            mapLines.getView().setCenter([JSON.parse(document.getElementById("center").innerText)[1], JSON.parse(document.getElementById("center").innerText)[0]]);
            mapLines.getView().setZoom(document.getElementById("zoom").innerText)
            sourceLines.refresh();
            sourceLines.addFeatures(new GeoJSON().readFeatures(JSON.parse(targetNodeLines.innerText)));
            mapIcons.getView().setCenter([JSON.parse(document.getElementById("center").innerText)[1], JSON.parse(document.getElementById("center").innerText)[0]]);
            mapIcons.getView().setZoom(document.getElementById("zoom").innerText)
            sourceIcons.refresh();
            sourceIcons.addFeatures(new GeoJSON().readFeatures(JSON.parse(targetNodeIcons.innerText)));
        }
    }
};

// Create an observer instance linked to the callback function
var observer = new MutationObserver(callback);

// Start observing the target node for configured mutations
observer.observe(targetNode, config);
observer.observe(targetNodeLines, config);
observer.observe(targetNodeIcons, config);

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

var tooltipIcons = document.getElementById('tooltipIcons');
var overlayIcons = new Overlay({
    element: tooltipIcons,
    offset: [10, 0],
    positioning: 'bottom-left'
});
mapIcons.addOverlay(overlayIcons);

function displayTooltipIcons(evt) {
    var pixel = evt.pixel;
    var feature = mapIcons.forEachFeatureAtPixel(pixel, function(feature) {
        return feature;
    });
    tooltipIcons.style.display = feature ? '' : 'none';
    if (feature) {
        overlayIcons.setPosition(evt.coordinate);
        tooltipIcons.innerHTML = feature.get('name');
    }
};

mapIcons.on('pointermove', displayTooltipIcons);

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

var exportOptions = {
    filter: function(element) {
        return element.className ? element.className.indexOf('ol-control') === -1 : true;
    }
};

document.getElementById("bPrintPDF").addEventListener("mousedown",function (){
    map.once('rendercomplete', function() {
        toPng(map.getTargetElement(), exportOptions)
            .then(function(dataURL) {
                var img = new Image();
                img.src = dataURL;
                document.getElementById("mapPointsImageImg").src = dataURL;
            });
    });

    mapLines.once('rendercomplete', function() {
        toPng(mapLines.getTargetElement(), exportOptions)
            .then(function(dataURL) {
                var img = new Image();
                img.src = dataURL;
                document.getElementById("mapLinesImageImg").src = dataURL;
            });
    });

    mapIcons.once('rendercomplete', function() {
        toPng(mapIcons.getTargetElement(), exportOptions)
            .then(function(dataURL) {
                var img = new Image();
                img.src = dataURL;
                document.getElementById("mapIconsImageImg").src = dataURL;
            });
    });

    map.renderSync();
    mapLines.renderSync();
    mapIcons.renderSync();
});