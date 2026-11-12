// A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1): the scene
// in Qt Quick 3D. main_quick3d.cpp moves the figures, the ball and the
// camera every frame. Not part of the application.

import QtQuick
import QtQuick3D

Item {
    id: root

    property url groundTexture
    property bool figures: true
    property bool shadows: true
    property var figureNodes: []
    property var cylinderNodes: []

    View3D {
        id: view
        objectName: "view"
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "#8cb8e0"
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            objectName: "camera"
            fieldOfView: 35
            clipNear: 0.5
            clipFar: 2000
        }

        DirectionalLight {
            objectName: "sun"
            castsShadow: root.shadows
            shadowMapQuality: Light.ShadowMapQualityVeryHigh
            shadowFactor: 60
            shadowMapFar: 400
            brightness: 0.9
            ambientColor: Qt.rgba(0.4, 0.4, 0.4, 1.0)
        }

        // 140 m x 100 m: the #Rectangle mesh is 100 x 100 in the xy plane
        Model {
            source: "#Rectangle"
            eulerRotation.x: -90
            scale: Qt.vector3d(1.4, 1.0, 1.0)
            castsShadows: false
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColorMap: Texture { source: root.groundTexture }
                roughness: 1.0
                metalness: 0.0
            }
        }

        Node { id: players }

        Model {
            objectName: "ball"
            source: "#Sphere" // diameter 100
            materials: PrincipledMaterial { baseColor: "white"; roughness: 0.6 }
        }
    }

    Component {
        id: figureComponent
        Mannequin { visible: root.figures }
    }
    Component {
        id: cylinderComponent
        Model {
            visible: !root.figures
            source: "#Cylinder" // radius 50, height 100, centered
            property color color: "white"
            materials: PrincipledMaterial { baseColor: color; roughness: 0.7 }
        }
    }

    Component.onCompleted: {
        var f = [], c = [];
        for (var i = 0; i < 22; ++i) {
            f.push(figureComponent.createObject(players));
            c.push(cylinderComponent.createObject(players));
        }
        figureNodes = f;
        cylinderNodes = c;
    }
}
