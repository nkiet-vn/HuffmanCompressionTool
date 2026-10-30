import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs
import "components" as Atelier

Window {
    id: window
    width: foundry.windowState().w
    height: foundry.windowState().h
    minimumWidth: 1024
    minimumHeight: 680
    visible: true
    title: "The Type Foundry  /  Huffman Compression Tool"
    flags: Qt.platform.os === "windows" ? Qt.Window | Qt.FramelessWindowHint : Qt.Window
    color: paper
    SystemPalette {id: systemPalette; colorGroup: SystemPalette.Active}
    property bool dark: foundry.theme === "dark" || (foundry.theme === "system" &&
                        systemPalette.window.r + systemPalette.window.g + systemPalette.window.b < 1.5)
    property color paper: dark ? "#0E0D0C" : "#F2EDE4"
    property color card: dark ? "#1A1714" : "#EAE3D7"
    property color ink: dark ? "#EDE6DA" : "#14110F"
    property color soft: dark ? "#AFAB9F" : "#6A645D"
    property color accent: dark ? "#FF5A36" : "#E8472B"
    property color verifiedInk: "#2F7D5B"
    property bool playing: false
    property int proofCharacters: 0
    function headerMeaning(key) {
        if(key==="magic")return "ASCII archive signature / 4 bytes"
        if(key==="mode")return "00 Huffman, 01 stored / 1 byte"
        if(key==="originalLength")return "Unsigned 64-bit LE size / 8 bytes"
        if(key==="crc32")return "CRC32 of original data, LE / 4 bytes"
        if(key==="uniqueCount")return "Distinct byte symbols, LE / 2 bytes"
        return "Byte symbol followed by canonical bit length / 2 bytes"
    }
    property bool overwriteOpen: false
    property bool themeOpen: false
    property bool langOpen: false
    property string hoveredHeader: ""
    function l(vi,en,ascii) {return foundry.language==="en"?en:foundry.language==="ascii"?ascii:vi}
    function beginOpen() { fileOpen.open() }
    function runAction() { if (foundry.screen <= 3) foundry.start() }
    onClosing: function(close) {foundry.setWindowState(x,y,width,height)}
    Component.onCompleted: {
        const state = foundry.windowState()
        if(state.x >= 0 && state.y >= 0) { window.x = state.x; window.y = state.y }
    }
    Connections {
        target: foundry
        function onConfirmOverwrite(){window.overwriteOpen=true}
        function onChanged(){ if(!foundry.verified)window.proofCharacters=0 }
    }
    Timer {interval:25;repeat:true;running:foundry.verified && window.proofCharacters<64
        onTriggered:window.proofCharacters++}
    FileDialog {
        id: fileOpen
        title: window.l("Chọn tệp để đưa vào máy in","Select input file","Chon tep dau vao")
        fileMode: FileDialog.OpenFile
        onAccepted: foundry.setSourceUrl(selectedFile)
    }
    FileDialog {
        id: fileSave
        title: window.l("Chọn đường dẫn đầu ra","Choose output path","Chon duong dan dau ra")
        fileMode: FileDialog.SaveFile
        onAccepted: foundry.setDestinationUrl(selectedFile)
    }
    Shortcut {sequence: "Ctrl+O"; onActivated: beginOpen()}
    Shortcut {sequence: "Ctrl+Return"; onActivated: runAction()}
    Shortcut {sequence: "Escape"; onActivated: {if(overwriteOpen)overwriteOpen=false;else foundry.cancel()}}
    Shortcut {sequence: "F1"; onActivated: foundry.screen=4}
    Repeater {
        model: 6
        delegate: Item {
            Shortcut {sequence: "Ctrl+"+(index+1); onActivated: foundry.screen=index}
            Shortcut {sequence: "Alt+"+(index+1); onActivated: foundry.screen=index}
            Shortcut {sequence: String(index+1); onActivated: {
                if(!window.activeFocusItem || window.activeFocusItem.cursorPosition === undefined)
                    foundry.screen=index
            }}
        }
    }
    Timer {
        interval: foundry.reduceMotion ? 50 : 460
        repeat: true; running: playing && foundry.traceIndex < foundry.trace.length-1
        onTriggered: foundry.advanceTrace()
    }
    Rectangle {anchors.fill:parent; color:paper}
    Image {
        anchors.fill:parent; source: "resources/grain.png"
        fillMode:Image.Tile; opacity:dark?.027:.038; visible: true
        asynchronous:true; cache:true
    }
    // Crop marks and reg marks are drawn as hairlines, not floating cards.
    Rectangle {x:17; y:15; width:23; height:1; color:ink; opacity:.35}
    Rectangle {x:17; y:15; width:1; height:23; color:ink; opacity:.35}
    Rectangle {x:window.width-39; y:window.height-18; width:22; height:1; color:ink; opacity:.35}
    Rectangle {x:window.width-18; y:window.height-38; width:1; height:21; color:ink; opacity:.35}

    Rectangle {
        id: titleStrip; anchors.top:parent.top; width:parent.width; height:59; z:100
        color:Qt.rgba(paper.r,paper.g,paper.b,.95)
        Rectangle {anchors.bottom:parent.bottom; height:1; width:parent.width; color:ink; opacity:.2}
        MouseArea {
            anchors.fill:parent
            onDoubleClicked: {if(window.visibility===Window.Maximized)window.showNormal();else window.showMaximized()}
            onPressed: { if(Qt.platform.os==="windows") foundry.moveWindow(window) }
        }
        Row {
            x:31; height:parent.height; spacing:18
            Text {text:"TF"; font.family:"Fraunces"; font.pixelSize:25; font.weight:Font.Black
                color:accent; anchors.verticalCenter:parent.verticalCenter}
            Rectangle {height:24; width:1; color:ink; opacity:.25; anchors.verticalCenter:parent.verticalCenter}
            Text {text:"THE TYPE FOUNDRY"; font.family:"IBM Plex Sans"; font.pixelSize:13
                font.bold:true; font.letterSpacing:2.8; color:ink; anchors.verticalCenter:parent.verticalCenter}
            Text {text:"/   HUFFMAN  •  v1.0.0"; font.family:"JetBrains Mono"; font.pixelSize:10
                color:soft; anchors.verticalCenter:parent.verticalCenter}
        }
        Row {
            anchors.right:windowButtons.left; anchors.rightMargin:14; anchors.verticalCenter:parent.verticalCenter; spacing:9
            Atelier.PressButton {label:foundry.theme==="system"?"◒ OS":foundry.theme==="dark"?"◐ DARK":"◑ PAPER"; compact:true
                paper:window.paper;ink:window.ink;accent:window.accent; onClicked: themeOpen=!themeOpen}
            Atelier.PressButton {label:foundry.language==="vi"?"VI":foundry.language==="en"?"EN":"ASCII"; compact:true
                paper:window.paper;ink:window.ink;accent:window.accent; onClicked:langOpen=!langOpen}
        }
        Row {
            id:windowButtons; anchors.right:parent.right; height:parent.height; spacing:0
            visible:Qt.platform.os==="windows"
            Repeater {
                model:["—","□","×"]
                delegate:Rectangle {
                    width:43;height:59;color:winButton.containsMouse?card:"transparent"
                    Text {anchors.centerIn:parent;text:modelData;color:ink;font.family:"IBM Plex Sans";font.pixelSize:18}
                    MouseArea {id:winButton; anchors.fill:parent;hoverEnabled:true;onClicked: {
                        if(index===0)window.showMinimized();else if(index===1){if(window.visibility===Window.Maximized)window.showNormal();else window.showMaximized()}
                        else window.close()
                    }}
                }
            }
        }
        Rectangle {
            visible:themeOpen; z:100; x:window.width-335; y:51; width:174;height:142
            border.color:ink; color:card
            Column {anchors.fill:parent; anchors.margins:5
                Repeater {model:[{id:"system",label:"Follow system"},{id:"paper",label:"Paper / light"},{id:"dark",label:"Darkroom"}]
                    delegate:Rectangle {width:parent.width;height:31;color:choiceArea.containsMouse?paper:"transparent"
                        Text {x:9;anchors.verticalCenter:parent.verticalCenter;text:modelData.label;color:ink;font.family:"IBM Plex Sans";font.pixelSize:12}
                        MouseArea {id:choiceArea;anchors.fill:parent;hoverEnabled:true;onClicked:{foundry.theme=modelData.id;themeOpen=false}}
                    }
                }
                Rectangle {width:parent.width;height:31;color:"transparent"
                    Text {x:9;anchors.verticalCenter:parent.verticalCenter;text:(foundry.reduceMotion?"[X] ":"[ ] ")+"Reduce motion"
                        color:ink;font.family:"IBM Plex Sans";font.pixelSize:12}
                    MouseArea {anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:{foundry.reduceMotion=!foundry.reduceMotion;themeOpen=false}}
                }
            }
        }
        Rectangle {
            visible:langOpen; z:101; x:window.width-210; y:51; width:170;height:106
            border.color:ink; color:card
            Column {anchors.fill:parent;anchors.margins:5
                Repeater {model:[{id:"vi",label:"Tiếng Việt"},{id:"en",label:"English"},{id:"ascii",label:"Không dấu / ASCII"}]
                    delegate:Rectangle {width:parent.width;height:31;color:langArea.containsMouse?paper:"transparent"
                        Text {x:9;anchors.verticalCenter:parent.verticalCenter;text:modelData.label;color:ink;font.family:"IBM Plex Sans";font.pixelSize:12}
                        MouseArea {id:langArea;anchors.fill:parent;hoverEnabled:true;onClicked:{foundry.language=modelData.id;langOpen=false}}
                    }
                }
            }
        }
    }
    Rectangle {
        id: rail; anchors.top:titleStrip.bottom; anchors.bottom:footer.top
        width:85; color:Qt.rgba(ink.r,ink.g,ink.b,.035)
        Rectangle {x:parent.width-1;width:1;height:parent.height;color:ink;opacity:.2}
        Column {
            anchors.top:parent.top; anchors.topMargin:26; anchors.horizontalCenter:parent.horizontalCenter;spacing:12
            Repeater {
                model:["SET","UNSET","PROOF","SCAN","HELP","ABOUT"]
                delegate:Item {
                    width:63;height:65
                    Rectangle {width:60;height:60;anchors.horizontalCenter:parent.horizontalCenter
                        color:foundry.screen===index?ink:"transparent"
                        border.color:foundry.screen===index?ink:Qt.rgba(ink.r,ink.g,ink.b,.25)
                    }
                    Text {anchors.horizontalCenter:parent.horizontalCenter;anchors.top:parent.top;anchors.topMargin:7
                        text:String(index+1).padStart(2,"0"); font.family:"JetBrains Mono";font.pixelSize:12
                        color:foundry.screen===index?accent:soft}
                    Text {anchors.horizontalCenter:parent.horizontalCenter;anchors.bottom:parent.bottom;anchors.bottomMargin:11
                        text:modelData;font.family:"IBM Plex Sans";font.bold:true;font.pixelSize:9
                        font.letterSpacing:.6;color:foundry.screen===index?paper:ink}
                    MouseArea {anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:foundry.screen=index}
                }
            }
        }
        Column {anchors.bottom:parent.bottom;anchors.bottomMargin:22;anchors.horizontalCenter:parent.horizontalCenter
            Text {text:"NO.";font.family:"JetBrains Mono";font.pixelSize:10;color:soft}
            Text {text:"03";font.family:"Fraunces";font.pixelSize:25;color:ink}
        }
    }
    Rectangle {
        id:footer; anchors.bottom:parent.bottom;height:45;width:parent.width;color:ink
        Text {x:26;anchors.verticalCenter:parent.verticalCenter;text:"TERMINAL /";color:accent
            font.family:"JetBrains Mono";font.pixelSize:10;font.bold:true}
        Text {id:terminalText; x:135; width:parent.width-240;clip:true;elide:Text.ElideMiddle
            anchors.verticalCenter:parent.verticalCenter; text:foundry.command
            color:paper;font.family:"JetBrains Mono";font.pixelSize:11}
        Rectangle {width:83;height:27;anchors.right:parent.right;anchors.rightMargin:22
            anchors.verticalCenter:parent.verticalCenter;color:accent
            Text {anchors.centerIn:parent;text:"COPY ↗";font.family:"IBM Plex Sans";font.pixelSize:11;font.bold:true;color:"white"}
            MouseArea {anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:foundry.copyText(foundry.command)}
        }
    }
    RowLayout {
        id: workspace
        anchors.top:titleStrip.bottom;anchors.bottom:footer.top;anchors.left:rail.right;anchors.right:parent.right
        spacing:0
        Rectangle {
            Layout.fillHeight:true;Layout.fillWidth:true;Layout.preferredWidth:48
            color:"transparent"
            Rectangle {anchors.right:parent.right;height:parent.height;width:1;color:ink;opacity:.18}
            Flickable {
                id:leftFlick; anchors.fill:parent;anchors.margins:30
                contentWidth:width; contentHeight:leftContent.implicitHeight+15
                clip:true;boundsBehavior:Flickable.StopAtBounds
                Column {
                    id:leftContent;width:leftFlick.width;spacing:16
                    Text {text:"THE FOUNDRY    /    OPERATION "+String(foundry.screen+1).padStart(2,"0")
                        font.family:"JetBrains Mono";font.pixelSize:10;font.letterSpacing:1.3;color:accent}
                    Text {text:foundry.titleForScreen(foundry.screen);width:parent.width;wrapMode:Text.WordWrap
                        font.family:"Fraunces";font.pixelSize:38;font.weight:Font.DemiBold;color:ink;lineHeight:.96}
                    Rectangle {width:parent.width;height:2;color:ink}
                    Text {text:foundry.helpForScreen(foundry.screen);width:parent.width;wrapMode:Text.WordWrap
                        font.family:"IBM Plex Sans";font.pixelSize:13;lineHeight:1.45;color:soft}
                    Item {width:1;height:3}
                    Column {
                        width:parent.width;spacing:12;visible:foundry.screen<=3
                        Atelier.RuleLabel {width:parent.width;text:"01  /  FEED STOCK";ink:window.ink;accent:window.accent}
                        Rectangle {
                            width:parent.width;height:155
                            color:dropArea.containsDrag?Qt.rgba(accent.r,accent.g,accent.b,.1):Qt.rgba(ink.r,ink.g,ink.b,.018)
                            border.width:dropArea.containsDrag?2:1
                            border.color:dropArea.containsDrag?accent:Qt.rgba(ink.r,ink.g,ink.b,.4)
                            Canvas {anchors.fill:parent;onPaint:{const ctx=getContext("2d");ctx.strokeStyle=window.ink;ctx.globalAlpha=.15;
                                for(let i=8;i<width;i+=17){ctx.beginPath();ctx.moveTo(i,0);ctx.lineTo(i-30,height);ctx.stroke()}}}
                            Text {anchors.top:parent.top;anchors.left:parent.left;anchors.margins:18;text:"[  +  ]"
                                color:accent;font.family:"JetBrains Mono";font.pixelSize:19;font.bold:true}
                            Text {anchors.left:parent.left;anchors.leftMargin:19;anchors.bottom:parent.bottom;anchors.bottomMargin:41
                                text:window.l("THẢ TỆP VÀO KHAY CHỮ","DROP FILE INTO THE TYPE CASE","THA TEP VAO KHAY")
                                font.family:"IBM Plex Sans";font.bold:true;font.pixelSize:14;color:ink}
                            Text {anchors.left:parent.left;anchors.leftMargin:19;anchors.bottom:parent.bottom;anchors.bottomMargin:19
                                text:window.l("Hoặc bấm để chọn từ máy","or click to select from disk","hoac bam de chon tep")
                                font.family:"IBM Plex Sans";font.pixelSize:12;color:soft}
                            DropArea {id:dropArea;anchors.fill:parent;onDropped:function(drop){
                                if(drop.urls.length>0){foundry.setSourceUrl(drop.urls[0]);drop.acceptProposedAction()}
                            }}
                            MouseArea {anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:beginOpen()}
                        }
                        Text {text:window.l("TỆP NGUỒN","SOURCE FILE","TEP NGUON");font.family:"IBM Plex Sans";font.pixelSize:10;font.bold:true;font.letterSpacing:1.4;color:soft}
                        Atelier.TypeField {width:parent.width;text:foundry.source
                            paper:window.paper;ink:window.ink;accent:window.accent;placeholder:"/path/to/source";onAccepted:value=>foundry.setSource(value)}
                        Row {
                            spacing:8;visible:foundry.screen<3
                            Text {text:window.l("TỆP ĐÍCH","OUTPUT FILE","TEP DICH");width:leftFlick.width-112
                                font.family:"IBM Plex Sans";font.pixelSize:10;font.bold:true;font.letterSpacing:1.3;color:soft;anchors.verticalCenter:parent.verticalCenter}
                            Atelier.PressButton {label:"BROWSE";compact:true;paper:window.paper;ink:window.ink;accent:window.accent;onClicked:fileSave.open()}
                        }
                        Atelier.TypeField {width:parent.width;visible:foundry.screen<3;text:foundry.destination
                            paper:window.paper;ink:window.ink;accent:window.accent;placeholder:"/path/to/output.bin";onAccepted:value=>foundry.setDestination(value)}
                        Text {visible:foundry.screen===2;text:"RESTORED COPY  /  "+foundry.destination+".restored";
                            width:parent.width;wrapMode:Text.WrapAnywhere;color:soft;font.family:"JetBrains Mono";font.pixelSize:10}
                        Item {width:1;height:3}
                        Row {spacing:10
                            Atelier.PressButton {label:foundry.busy?"PRESS RUNNING":foundry.screen===1?"DECOMPRESS":foundry.screen===2?"COMPRESS + VERIFY":foundry.screen===3?"INSPECT HEADER":"COMPRESS FILE"
                                width:200;primary:true;enabled:!foundry.busy && foundry.source.length>0
                                paper:window.paper;ink:window.ink;accent:window.accent;onClicked:runAction()}
                            Atelier.PressButton {label:"CANCEL";compact:false;enabled:foundry.busy;paper:window.paper;ink:window.ink;accent:window.accent;onClicked:foundry.cancel()}
                        }
                        Rectangle {width:parent.width;height:1;color:ink;opacity:.2}
                        Row {width:parent.width;spacing:12
                            Rectangle {height:29;width:88;color:foundry.verified?verifiedInk:
                                    foundry.status==="error"?accent:ink
                                Text {anchors.centerIn:parent;text:foundry.status.toUpperCase();color:paper;font.pixelSize:10;font.bold:true;font.family:"IBM Plex Sans"}}
                            Text {text:foundry.stage;anchors.verticalCenter:parent.verticalCenter;color:ink;font.family:"JetBrains Mono";font.pixelSize:10}
                        }
                        Text {text:foundry.detail;width:parent.width;wrapMode:Text.WordWrap;color:soft
                            font.family:"IBM Plex Sans";font.pixelSize:12;visible:text.length>0}
                        Rectangle {width:parent.width;height:22;visible:foundry.busy;color:"transparent";border.color:ink
                            Rectangle {width:parent.width*foundry.progress/100;height:parent.height;color:accent
                                Behavior on width {enabled:!foundry.reduceMotion;NumberAnimation{duration:180}}}
                            Text {anchors.centerIn:parent;text:foundry.progress+"%";color:ink;font.family:"JetBrains Mono";font.pixelSize:10;font.bold:true}
                        }
                        Atelier.PressButton {visible:foundry.status==="success";width:190;label:"SHOW IN FOLDER";compact:true
                            paper:window.paper;ink:window.ink;accent:window.accent;onClicked:foundry.showInFolder()}
                    }
                    Column {width:parent.width;spacing:8;visible:foundry.screen===4
                        Atelier.RuleLabel {width:parent.width;text:"THE TERMINAL PRIMER";ink:window.ink}
                        Repeater {
                            model:[{"flag":"-c","meaning":"Compress a file to HUF3","usage":"huffman -c source.txt packed.bin"},
                                   {"flag":"-d","meaning":"Decompress HUF1, HUF2 or HUF3","usage":"huffman -d packed.bin restored.txt"},
                                   {"flag":"-v","meaning":"Compress, restore, verify SHA-256","usage":"huffman -v original.txt packed.bin"},
                                   {"flag":"-i","meaning":"Inspect file format and SHA-256","usage":"huffman -i packed.bin"},
                                   {"flag":"--menu","meaning":"Open the original console menu","usage":"huffman --menu"},
                                   {"flag":"--ascii","meaning":"Vietnamese without diacritics","usage":"huffman --ascii --menu"},
                                   {"flag":"--help","meaning":"Print all command-line options","usage":"huffman --help"}]
                            delegate:Rectangle {
                                width:leftFlick.width;height:69;color:"transparent";border.color:Qt.rgba(ink.r,ink.g,ink.b,.18)
                                Text {x:12;y:8;text:modelData.flag;color:accent;font.family:"JetBrains Mono";font.pixelSize:14;font.bold:true}
                                Text {x:90;y:9;text:modelData.meaning;color:ink;font.family:"IBM Plex Sans";font.pixelSize:12}
                                Text {x:12;y:42;text:modelData.usage;color:soft;font.family:"JetBrains Mono";font.pixelSize:10}
                                Text {anchors.right:parent.right;anchors.rightMargin:10;y:42;text:"COPY";color:accent;font.family:"IBM Plex Sans";font.pixelSize:10}
                                MouseArea {anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:foundry.copyText(modelData.usage)}
                            }
                        }
                    }
                    Column {width:parent.width;spacing:15;visible:foundry.screen===5
                        Atelier.RuleLabel {width:parent.width;text:"MASTER PLATE / V1.0.0";ink:window.ink}
                        Text {width:parent.width;wrapMode:Text.WordWrap;color:ink;font.family:"Fraunces";font.pixelSize:26
                            text:"A smaller alphabet in the same world of bytes."}
                        Text {width:parent.width;wrapMode:Text.WordWrap;color:soft;font.family:"IBM Plex Sans";font.pixelSize:13
                            text:"A native C++17 compression engine with a Qt Quick observation bench. No cloud processing, telemetry or external service. All files remain on your machine."}
                        Text {width:parent.width;wrapMode:Text.WordWrap;color:soft;font.family:"JetBrains Mono";font.pixelSize:11
                            text:"TREE        Custom binary Min-Heap\nKEY         (frequency, minimum symbol)\nCODEBOOK    Canonical Huffman, max 32 bits\nBITS        MSB-first read / write\nINTEGRITY   CRC32 IEEE 802.3\nPROOF       SHA-256 original / restored\nARCHIVE     HUF3 | compatible HUF1, HUF2"}
                    }
                    Column {
                        width:parent.width;spacing:7;visible:foundry.recentFiles.length>0 && foundry.screen<4
                        Atelier.RuleLabel {width:parent.width;text:"RECENT STOCK  /  REGISTER";ink:window.ink}
                        Repeater {model:foundry.recentFiles
                            delegate:Text {width:leftFlick.width;elide:Text.ElideMiddle
                                text:"0"+(index+1)+" / "+modelData;color:soft
                                font.family:"JetBrains Mono";font.pixelSize:10
                                MouseArea {anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:foundry.setSource(modelData)}
                            }
                        }
                    }
                }
            }
        }
        Rectangle {
            Layout.fillHeight:true;Layout.fillWidth:true;Layout.preferredWidth:52
            color:Qt.rgba(ink.r,ink.g,ink.b,.018)
            Flickable {
                id:benchFlick;anchors.fill:parent;anchors.margins:28
                contentWidth:width;contentHeight:benchColumn.implicitHeight+18;clip:true
                boundsBehavior:Flickable.StopAtBounds
                Column {
                    id:benchColumn;width:benchFlick.width;spacing:17
                    Row {width:parent.width
                        Text {text:"OBSERVATION BENCH";font.family:"IBM Plex Sans";font.pixelSize:11;font.bold:true
                            font.letterSpacing:2;color:ink}
                        Text {text:"                                   LIVE / 001";font.family:"JetBrains Mono";font.pixelSize:10;color:soft}
                    }
                    Rectangle {width:parent.width;height:1;color:ink;opacity:.33}
                    // Real byte frequency observations, scanned in worker.
                    Column {
                        width:parent.width;spacing:10;visible:foundry.screen===0 || foundry.screen===2
                        Atelier.RuleLabel {width:parent.width;text:"I   /   SORT CASE  ·  BYTE FREQUENCY";ink:window.ink}
                        Text {visible:foundry.histogram.length===0;text:"AWAITING TYPE / DROP A SOURCE FILE";color:soft
                            font.family:"JetBrains Mono";font.pixelSize:11}
                        Rectangle {
                            width:parent.width;height:182;visible:foundry.histogram.length>0
                            color:"transparent";border.color:Qt.rgba(ink.r,ink.g,ink.b,.22)
                            Row {
                                anchors.fill:parent;anchors.margins:10;spacing:3
                                Repeater {
                                    model:foundry.histogram.slice(0,48)
                                    delegate:Item {
                                        width: Math.max(4,(benchFlick.width-25)/Math.max(1,Math.min(foundry.histogram.length,48))-3)
                                        height:parent.height
                                        Rectangle {
                                            width:parent.width;anchors.bottom:parent.bottom
                                            height:Math.max(2,modelData.fraction*(parent.height-15))
                                            color:accent;opacity:.9
                                            Behavior on height {enabled:!foundry.reduceMotion;NumberAnimation{duration:420;easing.type:Easing.OutCubic}}
                                        }
                                    }
                                }
                            }
                        }
                        Text {text:foundry.histogram.length+" UNIQUE SORTS / HIGHEST FREQUENCY FIRST";color:soft
                            font.family:"JetBrains Mono";font.pixelSize:10}
                        Atelier.RuleLabel {width:parent.width;text:"II   /   CASTING THE FORME";ink:window.ink}
                        Rectangle {
                            width:parent.width;height:112;color:card;border.color:Qt.rgba(ink.r,ink.g,ink.b,.25)
                            Column {anchors.fill:parent;anchors.margins:13;spacing:7
                                Text {text:foundry.trace.length===0?"NO NODES TO CAST":foundry.traceIndex<0?"HEAP / READY TO CAST":foundry.trace[foundry.traceIndex].description
                                    font.family:"JetBrains Mono";font.pixelSize:11;color:ink}
                                Row {spacing:8
                                    Atelier.PressButton {label:playing?"PAUSE":"PLAY";compact:true;paper:window.paper;ink:window.ink;accent:window.accent
                                        onClicked:{if(foundry.traceIndex>=foundry.trace.length-1)foundry.traceIndex=-1;playing=!playing}}
                                    Atelier.PressButton {label:"<";compact:true;width:43;paper:window.paper;ink:window.ink;accent:window.accent;onClicked:foundry.retreatTrace()}
                                    Atelier.PressButton {label:">";compact:true;width:43;paper:window.paper;ink:window.ink;accent:window.accent;onClicked:foundry.advanceTrace()}
                                    Text {anchors.verticalCenter:parent.verticalCenter;text:(foundry.traceIndex+1)+" / "+foundry.trace.length
                                        font.family:"JetBrains Mono";font.pixelSize:10;color:soft}
                                }
                                Rectangle {width:parent.width;height:7;color:Qt.rgba(ink.r,ink.g,ink.b,.14)
                                    Rectangle {width:parent.width*(foundry.traceIndex+1)/Math.max(1,foundry.trace.length)
                                        height:parent.height;color:accent}
                                    MouseArea {anchors.fill:parent;onClicked:function(mouse){
                                        foundry.traceIndex=Math.floor(mouse.x/width*foundry.trace.length)-1
                                    }}
                                }
                            }
                        }
                        Text {visible:foundry.traceIndex>=0 && foundry.trace[foundry.traceIndex].tie
                            text:"TIE BREAK / SAME FREQUENCY → SMALLER minSymbol FIRST";color:accent
                            font.family:"JetBrains Mono";font.pixelSize:10}
                        ListView {
                            width:parent.width;height:Math.max(0,Math.min(104,contentHeight))
                            model:foundry.heapModel;clip:true;interactive:false
                            delegate:Row {
                                width:ListView.view.width;height:29;spacing:12
                                Text {width:150;text:symbolLabel;color:accent;font.family:"JetBrains Mono";font.pixelSize:11}
                                Text {text:keyValue;color:ink;font.family:"JetBrains Mono";font.pixelSize:11}
                            }
                        }
                        Atelier.RuleLabel {width:parent.width;text:"III   /   BIT RIBBON / ACTUAL SOURCE BYTES";ink:window.ink}
                        Rectangle {width:parent.width;height:91;color:card;border.color:Qt.rgba(ink.r,ink.g,ink.b,.25)
                            Column {anchors.fill:parent;anchors.margins:11;spacing:6
                                Text {text:"8-BIT INK   /  "+(foundry.fileInfo.sampleByteCount||0)+" BYTES";color:soft
                                    font.family:"JetBrains Mono";font.pixelSize:9}
                                Text {text:foundry.fileInfo.originalRibbon||"—";width:parent.width;elide:Text.ElideRight
                                    color:ink;font.family:"JetBrains Mono";font.pixelSize:10}
                                Rectangle {width:parent.width;height:1;color:ink;opacity:.2}
                                Text {text:foundry.fileInfo.compressedRibbon||"—";width:parent.width;elide:Text.ElideRight
                                    color:accent;font.family:"JetBrains Mono";font.pixelSize:11}
                            }
                        }
                        Atelier.RuleLabel {width:parent.width;text:"IV   /   CANONICAL IMPRESSION";ink:window.ink}
                        Row {spacing:10
                            Text {text:"SORT";width:85;color:soft;font.family:"JetBrains Mono";font.pixelSize:10}
                            Text {text:"FREQ";width:96;color:soft;font.family:"JetBrains Mono";font.pixelSize:10}
                            Text {text:"LENGTH";width:67;color:soft;font.family:"JetBrains Mono";font.pixelSize:10}
                            Text {text:"BITS";color:soft;font.family:"JetBrains Mono";font.pixelSize:10}
                        }
                        ListView {
                            width:parent.width;height:Math.max(0,Math.min(145,contentHeight));clip:true;model:foundry.codeModel
                            delegate:Rectangle {
                                width:ListView.view.width;height:28;color:index%2===0?Qt.rgba(ink.r,ink.g,ink.b,.035):"transparent"
                                Row {anchors.verticalCenter:parent.verticalCenter;spacing:10
                                    Text {text:symbolLabel;width:85;color:ink;font.family:"JetBrains Mono";font.pixelSize:11}
                                    Text {text:frequency;width:96;color:ink;font.family:"JetBrains Mono";font.pixelSize:11}
                                    Text {text:codeLength;width:67;color:ink;font.family:"JetBrains Mono";font.pixelSize:11}
                                    Text {text:binaryCode;color:accent;font.family:"JetBrains Mono";font.pixelSize:11}
                                }
                            }
                        }
                        Text {text:foundry.fileInfo.special||"LONGER CODES FOR RARE BYTES. SHORTER CODES FOR FREQUENT BYTES."
                            wrapMode:Text.WordWrap;width:parent.width;font.family:"IBM Plex Sans";font.pixelSize:11;color:soft}
                    }
                    Column {
                        width:parent.width;spacing:13;visible:foundry.screen===1
                        Atelier.RuleLabel {width:parent.width;text:"DECOMPRESSION STAGES";ink:window.ink}
                        Repeater {model:["01   READ HEADER / HUF1 · HUF2 · HUF3","02   REBUILD CANONICAL CODEBOOK","03   DECODE PAYLOAD BITS","04   CRC32 INTEGRITY CHECK"]
                            delegate:Rectangle {width:parent.width;height:51;color:"transparent";border.color:Qt.rgba(ink.r,ink.g,ink.b,.2)
                                Text {anchors.centerIn:parent;text:modelData;color:index===3&&foundry.status==="success"?verifiedInk:ink
                                    font.family:"JetBrains Mono";font.pixelSize:12}
                            }
                        }
                    }
                    Column {
                        width:parent.width;spacing:10;visible:foundry.screen===2
                        Atelier.RuleLabel {width:parent.width;text:"CERTIFICATE NO. 256 / RETURN PROOF";ink:window.ink}
                        Rectangle {
                            width:parent.width;height:foundry.originalHash.length>0?249:120;color:paper
                            border.color:Qt.rgba(ink.r,ink.g,ink.b,.5)
                            Column {anchors.fill:parent;anchors.margins:16;spacing:13
                                Text {text:"PRINTER'S RECEIPT  /  VERIFICATION";font.family:"JetBrains Mono";font.pixelSize:10;color:soft}
                                Text {text:"ORIGINAL    /  "+(foundry.originalHash||"—");width:parent.width;wrapMode:Text.WrapAnywhere
                                    font.family:"JetBrains Mono";font.pixelSize:11;color:ink}
                                Text {text:"RESTORED    /  "+(foundry.restoredHash||"—");width:parent.width;wrapMode:Text.WrapAnywhere
                                    font.family:"JetBrains Mono";font.pixelSize:11;color:ink}
                                Row {visible:foundry.verified;spacing:1
                                    Repeater {model:foundry.restoredHash.split("")
                                        delegate:Text {text:modelData;color:verifiedInk
                                            font.family:"JetBrains Mono";font.pixelSize:9
                                            font.underline:index<window.proofCharacters}}
                                }
                                Rectangle {width:parent.width;height:1;color:ink;opacity:.4}
                                Text {text:foundry.verified?window.l("KHÔI PHỤC 100%","RESTORED 100%","KHOI PHUC 100%"):
                                            foundry.status==="error"?window.l("KHÔNG KHỚP","DOES NOT MATCH","KHONG KHOP"):"AWAITING VERIFICATION"
                                    font.family:"Fraunces";font.pixelSize:22;font.bold:true;color:foundry.verified?verifiedInk:foundry.status==="error"?accent:soft}
                            }
                            Row {anchors.bottom:parent.bottom;anchors.bottomMargin:-3;spacing:10
                                Repeater {model:Math.floor(parent.parent.width/11)
                                    delegate:Rectangle {width:4;height:4;color:card;rotation:45}}
                            }
                        }
                    }
                    Column {
                        width:parent.width;spacing:12;visible:foundry.screen===3
                        Atelier.RuleLabel {width:parent.width;text:"HUF HEADER / FIELD ATLAS";ink:window.ink}
                        Text {text:foundry.fileInfo.format||"NOT INSPECTED";color:accent;font.family:"Fraunces";font.pixelSize:28}
                        Text {text:"BYTE OFFSETS / HOVER A SEGMENT TO EXAMINE THE ACTUAL HEX";color:soft
                            font.family:"JetBrains Mono";font.pixelSize:10;wrapMode:Text.WordWrap;width:parent.width}
                        Row {
                            spacing:3;height:79
                            Repeater {
                                model:(foundry.fileInfo.segments||[]).slice(0,foundry.fileInfo.version===3 && foundry.fileInfo.mode===0?5:4)
                                delegate:Rectangle {
                                    width:Math.max(50,Math.min(130,20+modelData.bytes*11));height:parent.height
                                    color:headerArea.containsMouse?accent:card;border.color:ink
                                    Column {anchors.centerIn:parent;spacing:5
                                        Text {anchors.horizontalCenter:parent.horizontalCenter;text:modelData.label
                                            color:headerArea.containsMouse?paper:ink;font.family:"JetBrains Mono";font.pixelSize:9}
                                        Text {anchors.horizontalCenter:parent.horizontalCenter;text:modelData.bytes+" B"
                                            color:headerArea.containsMouse?paper:soft;font.family:"JetBrains Mono";font.pixelSize:12;font.bold:true}
                                        Text {anchors.horizontalCenter:parent.horizontalCenter;text:"@"+modelData.offset
                                            color:headerArea.containsMouse?paper:soft;font.family:"JetBrains Mono";font.pixelSize:9}
                                    }
                                    MouseArea {id:headerArea;anchors.fill:parent;hoverEnabled:true;
                                        onEntered:hoveredHeader=modelData.label+"  @"+modelData.offset+"   "+modelData.hex+"
"+window.headerMeaning(modelData.label)
                                        onExited:hoveredHeader=""}
                                }
                            }
                        }
                        Rectangle {width:parent.width;height:51;color:ink
                            Text {anchors.fill:parent;anchors.margins:9;text:hoveredHeader||"MAGIC / MODE / LENGTH / CRC32 / SYMBOL TABLE"
                                color:paper;font.family:"JetBrains Mono";font.pixelSize:11;wrapMode:Text.WrapAnywhere}
                        }
                        Text {text:"SHA-256 / "+(foundry.fileInfo.sha256||"—");wrapMode:Text.WrapAnywhere;width:parent.width
                            font.family:"JetBrains Mono";font.pixelSize:11;color:ink}
                        Text {text:"ARCHIVE BYTES   /  "+(foundry.fileInfo.fileSize||0)+
                              "\nORIGINAL BYTES  /  "+(foundry.fileInfo.originalLength||0)+
                              "\nSYMBOLS         /  "+(foundry.fileInfo.symbolCount||0)+
                              "\nSIZE REDUCTION  /  "+Number(foundry.fileInfo.ratio||0).toFixed(2)+"%"+
                              "\nCRC32           /  "+(foundry.fileInfo.crc||"—")
                            font.family:"JetBrains Mono";font.pixelSize:12;color:ink;lineHeight:1.5}
                        Atelier.RuleLabel {width:parent.width;text:"FIELD DIRECTORY / ACTUAL BYTES";ink:window.ink}
                        Repeater {
                            model:foundry.fileInfo.segments||[]
                            delegate:Row {width:benchFlick.width;height:23;spacing:8
                                Text {width:113;elide:Text.ElideRight;text:modelData.label;color:soft;font.family:"JetBrains Mono";font.pixelSize:10}
                                Text {width:42;text:"@"+modelData.offset;color:accent;font.family:"JetBrains Mono";font.pixelSize:10}
                                Text {width:benchFlick.width-164;elide:Text.ElideRight;text:modelData.hex;color:ink;font.family:"JetBrains Mono";font.pixelSize:10}
                            }
                        }
                    }
                    Column {
                        width:parent.width;spacing:15;visible:foundry.screen===4
                        Atelier.RuleLabel {width:parent.width;text:"OPERATOR'S QUICK LEGEND";ink:window.ink}
                        Text {width:parent.width;wrapMode:Text.WordWrap;color:ink;font.family:"Fraunces";font.pixelSize:30
                            text:"Each command is a tool. Each tool leaves a trace."}
                        Text {width:parent.width;wrapMode:Text.WordWrap;color:soft;font.family:"IBM Plex Sans";font.pixelSize:13
                            text:"Windows: build with MSYS2 UCRT64 and Qt 6. Run huffman_gui for this interface, or huffman --menu for the original terminal program. Binary archives are compatible."}
                        Text {width:parent.width;wrapMode:Text.WordWrap;color:ink;font.family:"JetBrains Mono";font.pixelSize:12
                            text:"CTRL + 1…6  /  SWITCH DESK\nCTRL + O     /  OPEN FILE\nCTRL + ENTER /  OPERATE\nESC          /  CANCEL\nF1           /  REFERENCE\nCLICK         /  COPY COMMAND"}
                    }
                    Column {
                        width:parent.width;spacing:12;visible:foundry.screen===5
                        Atelier.RuleLabel {width:parent.width;text:"SECTION / ENGINEERING DIAGRAM";ink:window.ink}
                        Canvas {
                            width:parent.width;height:275
                            onPaint: {
                                const ctx=getContext("2d");ctx.clearRect(0,0,width,height)
                                ctx.strokeStyle=window.ink;ctx.fillStyle=window.ink;ctx.lineWidth=1
                                const labels=["ConsoleUI","MenuController","Compressor / Decompressor","MinHeap → Huffman Tree","BitWriter / BitReader","HUF3 | CRC32 | SHA256"]
                                ctx.font="11px JetBrains Mono";ctx.textAlign="center"
                                for(let i=0;i<labels.length;i++) {
                                    const x=width/2-145,y=i*44+4
                                    ctx.strokeRect(x,y,290,31);ctx.fillText(labels[i],width/2,y+20)
                                    if(i<labels.length-1){ctx.beginPath();ctx.moveTo(width/2,y+31);ctx.lineTo(width/2,y+43);ctx.stroke()}
                                }
                            }
                        }
                        Text {text:"CORE / C++17  —  VIEW / QT QUICK";color:accent;font.family:"JetBrains Mono";font.pixelSize:11}
                    }
                    Column {
                        width:parent.width;spacing:11;visible:foundry.screen<=2 && foundry.status==="success"
                        Atelier.RuleLabel {width:parent.width;text:"FINAL IMPRESSION / RESULT";ink:window.ink}
                        Text {text:"ORIGINAL      "+(foundry.fileInfo.originalSize||0)+" bytes\nARCHIVE       "+(foundry.fileInfo.compressedSize||0)+
                              " bytes\nFORMAT        "+(foundry.fileInfo.format||"HUF3")+"\nSPACE SAVED   "+
                              Number(foundry.fileInfo.ratio||0).toFixed(2)+"%"
                            font.family:"JetBrains Mono";font.pixelSize:12;color:ink;lineHeight:1.5}
                    }
                }
            }
        }
    }
    // Native drag from Finder/Explorer is accepted by the whole window as well.
    DropArea {anchors.fill:workspace;z:-1;onDropped:function(drop){if(drop.urls.length){foundry.setSourceUrl(drop.urls[0]);drop.acceptProposedAction()}}}
    Rectangle {
        id: modalShade;anchors.fill:parent;visible:overwriteOpen;z:200
        color:Qt.rgba(0,0,0,.58)
        MouseArea {anchors.fill:parent;onClicked:overwriteOpen=false}
        Rectangle {
            width:440;height:234;anchors.centerIn:parent;color:paper;border.width:2;border.color:ink
            Column {anchors.fill:parent;anchors.margins:25;spacing:14
                Text {text:"OVERPRINT WARNING   /   001";font.family:"JetBrains Mono";font.pixelSize:11;color:accent}
                Text {text:window.l("Tệp đích đã tồn tại.","The output already exists.","Tep dich da ton tai.")
                    font.family:"Fraunces";font.pixelSize:26;color:ink}
                Text {width:parent.width;wrapMode:Text.WordWrap
                    text:window.l("Ghi đè sẽ thay thế nội dung hiện tại. Mặc định: Không.",
                                  "Continuing will replace the existing file. Default: No.",
                                  "Ghi de se thay the noi dung. Mac dinh: Khong.")
                    font.family:"IBM Plex Sans";font.pixelSize:13;color:soft}
                Row {spacing:10
                    Atelier.PressButton {label:"NO / KEEP FILE";paper:window.paper;ink:window.ink;accent:window.accent
                        onClicked:overwriteOpen=false}
                    Atelier.PressButton {label:"YES / REPLACE";primary:true;paper:window.paper;ink:window.ink;accent:window.accent
                        onClicked:{overwriteOpen=false;foundry.approveOverwrite()}}
                }
            }
        }
    }
    // Edge resize through OS window manager; snap behaviour remains OS-owned.
    MouseArea {visible:Qt.platform.os==="windows";anchors.left:parent.left;width:5;height:parent.height;z:110
        cursorShape:Qt.SizeHorCursor;onPressed:foundry.resizeWindow(window,Qt.LeftEdge)}
    MouseArea {visible:Qt.platform.os==="windows";anchors.right:parent.right;width:5;height:parent.height;z:110
        cursorShape:Qt.SizeHorCursor;onPressed:foundry.resizeWindow(window,Qt.RightEdge)}
    MouseArea {visible:Qt.platform.os==="windows";anchors.bottom:parent.bottom;width:window.width;height:5;z:110
        cursorShape:Qt.SizeVerCursor;onPressed:foundry.resizeWindow(window,Qt.BottomEdge)}
}
