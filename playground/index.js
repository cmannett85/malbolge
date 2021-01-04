/* Cam Mannett 2020
 *
 * See LICENSE file
 */

 /** Allocates fromthe heap and zeroes out the allocation.
  * 
  * @param {*} size Number of bytes to allocate
  * @return Pointer to start of allocated region
  */
 function zmalloc(size) {
    let ptr = _malloc(size);
    for (i = 0; i < size; ++i) {
        GROWABLE_HEAP_I8()[ptr + i] = 0;
    }

    return ptr;
 }

/** A simple wrapper around a raw C-String as managed by the WASM heap memory.
 */
class CString {
    /** Constructor.
     * 
     * @param {string} s JS string
     */
    constructor(s) {
        this._size = 0;
        this._ptr = null;

        let size = lengthBytesUTF8(s) + 1;
        let ret = _malloc(size);
        if (ret) {
            stringToUTF8(s, ret, size);
        }

        this._size = size - 1;
        this._ptr = ret;
    }

    /** Returns a pointer to the start of the array.
     *
     * @return C-String pointer 
     */
    get pointer() {
        return this._ptr;
    }

    /** Returns the size of the array.
     * 
     * @return Number of characters
     */
    get size() {
        return this._size;
    }

    /** Converts the WASM heap pointer and size back to a JS string.
     * 
     * @return JS string
     */
    toString() {
        return UTF8ToString(this._ptr, this._size);
    }

    /** Frees the data from the WASM heap.
     */
    destroy() {
        _free(this._ptr);
    }
}

/** Logging sources.
 */
const LogSource = {
    "malbolgeWasm": 0,  ///< Logging from the C++/WASM code
    "malbolgeJS":   1,  ///< Logging from Malbolge's JS code
    "emscripten":   2   ///< Logging from Emscripten's runtime
};

/** Log levels from Malbolge's JS code.
 */
const JSLogLevel = {
    "info":     0,  ///< Informational messages
    "error":    1   ///< Error messages
}

/** vCPU execution states.
 */
const vCPUExecutionState = {
    "ready":            0,  ///< Ready to run
    "running":          1,  ///< Program running
    "paused":           2,  ///< Program paused
    "waitingForInput":  3,  ///< Similar to paused, except the program will
                            ///< resume when input data provided
    "stopped":          4   ///< Program stopped, cannot be resumed or ran again
};

/** Program load normalised modes.
 */
const loadNormalisedMode = {
    "auto": 0,  ///< Automatically detect if normalised, uses malbolge_is_likely_normalised_source
    "on":   1,  ///< Force load as normalised
    "off":  2   ///< Force load as non-normalised
};

/** Sets a running state.
 * 
 * This enables/disables components depending on @a isRunning.
 * @param {boolean} isRunning True to represent a state where the Malbolge
 * program is running
 */
function programRunning(isRunning) {
    if (isRunning) {
        document.getElementById("programButton").innerText = "Stop";
    } else {
        if (Module.malbolgeProgramNormalised) {
            document.getElementById("programButton").innerText = "Run Normalised";
        } else {
            document.getElementById("programButton").innerText = "Run";
        }
        document.getElementById("inputButton").disabled = true;
    }

    document.getElementById("programButton").value = isRunning ? "stop" : "run";
    document.getElementById("preset").disabled = isRunning;
    document.getElementById("program").disabled = isRunning;
    
    document.getElementById("runTooltip").innerText = isRunning ?
        "Stop the executing program" :
        "Execute the program"
}

/** Returns true if the program is running.
 * 
 * @return True if running
 */
function isProgramRunning() {
    return document.getElementById("programButton").innerText == "Stop";
}

/** Wrap the original message handling with support for our own messages
 * 
 * There's no native support (yet) in Emscripten for doing this, hence this
 * hackery.
 */
function wrapWorkerHandler() {
    let worker = Module["PThread"].runningWorkers.slice(-1)[0];

    let standardHandlers = worker.onmessage;
    worker.onmessage = function (e) {
        if (e.data.cmd === "malbolgevCPUState") {
            if (e.data.state == vCPUExecutionState.stopped) {
                programRunning(false);
                if (Module.malbolgeVcpu) {
                    _malbolge_free_vcpu(Module.malbolgeVcpu);
                    Module.malbolgeVcpu = undefined;
                }
            } else if (e.data.state == vCPUExecutionState.waitingForInput) {
                document.getElementById("inputButton").disabled = false;
                console.log("Waiting for input");
            }
        } else if (e.data.cmd === "malbolgeOutput") {
            document.getElementById("output").value += e.data.data;
        } else {
            standardHandlers(e);
        }
    };
}

/** If there is program text present enables certain controls.
 * 
 * It also scans the input to determine if it is normalised, and configures the
 * normalisation dropdown appropriately.
 */
function programTextInput() {
    const programText = document.getElementById("program").value;
    const isEmpty = programText.length == 0;

    document.getElementById("normalise").disabled = isEmpty;
    document.getElementById("programButton").disabled = isEmpty;

    if (isEmpty) {
        return;
    }

    const programCText = new CString(programText);

    // First discover if the program is normalised or not
    const normalised = _malbolge_is_likely_normalised_source(programCText.pointer,
        programCText.size);
    programCText.destroy();

    // Select the appropriate normalise dropdown
    Module.malbolgeProgramNormalised = normalised;
    document.getElementById("programButton").innerText = normalised ? "Run Normalised" : "Run";
}

/** Normalises or denormalises the program source depending on the corresponding
 * dropdown box selection.
 */
function normaliseProgram() {
    const selectedIndex = document.getElementById("normalise").selectedIndex;
    if (selectedIndex == 0) {
        return;
    }

    const programText = new CString(document.getElementById("program").value);
    const normalise = selectedIndex == 1;

    if (normalise) {
        let newSizePtr = zmalloc(8); // Size is 64bit

        const ret = _malbolge_normalise_source(programText.pointer,
            programText.size,
            newSizePtr,
            0,
            0);
        if (ret == 0) {
            let newSize = GROWABLE_HEAP_I32()[newSizePtr >> 2] << 32;
            newSize += GROWABLE_HEAP_I32()[(newSizePtr + 4) >> 2];

            const normalisedProgram = UTF8ToString(programText.pointer, newSize);
            document.getElementById("program").value = normalisedProgram;
            programTextInput();
        }
        _free(newSizePtr);
    } else {
        const ret = _malbolge_denormalise_source(programText.pointer,
            programText.size,
            0);
        if (ret == 0) {
            const denormalisedProgram = programText.toString();
            document.getElementById("program").value = denormalisedProgram;
            programTextInput();
        }
    }

    document.getElementById("normalise").selectedIndex = 0;
}

/** Execute or stop the program as provided by the "output" element
 */
function runStopMalbolge() {
    if (isProgramRunning()) {
        stopMalbolge();
        return;
    }

    programRunning(true);
    document.getElementById("output").value = "";
    const programText = new CString(document.getElementById("program").value);
    if (programText.pointer == null) {
        consoleWrite("Failed to allocate program source heap",
            LogSource.malbolgeJS,
            JSLogLevel.error);
        programRunning(false);
        return;
    }

    // Load and validate the program
    vmem = _malbolge_load_program(programText.pointer,
        programText.size,
        Module.malbolgeProgramNormalised ?
            loadNormalisedMode.on :
            loadNormalisedMode.off,
        0,
        0);
    programText.destroy();

    if (!vmem) {
        programRunning(false);
        return;
    }

    // Create a vCPU, this will free vmem
    let vcpu = _malbolge_create_vcpu(vmem);
    if (!vcpu) {
        programRunning(false);
        return;
    }

    let err = _malbolge_vcpu_run_wasm(vcpu);
    if (err) {
        programRunning(false);
        _malbolge_free_vcpu(vcpu);
        return;
    }

    wrapWorkerHandler();

    Module.malbolgeVcpu = vcpu;
}

/** Stop the Malbolge program execution.
 */
function stopMalbolge() {
    programRunning(false);
    if (Module.malbolgeVcpu) {
        _malbolge_free_vcpu(Module.malbolgeVcpu);
        Module.malbolgeVcpu = undefined;
    }
}

/** Passes the input text provided by the "input" element to the currently
 * running program.
 */
function setInputText() {
    let text = document.getElementById("input").value;
    if (text.length) {
        const ctext = new CString(text + "\n");
        if (ctext.pointer == null) {
            consoleWrite("Failed to allocate input text heap",
                LogSource.malbolgeJS,
                JSLogLevel.error);
            return;
        }

        _malbolge_vcpu_add_input(Module.malbolgeVcpu, ctext.pointer, ctext.size);
        ctext.destroy();

        document.getElementById("input").value = "";
        document.getElementById("inputButton").disabled = true;
    }
}

/** Callback from the Emscripten runtime to notify that the WASM has finished
 * loading.
 */
function finishedLoading() {
    // Print the version once the WASM initialisation has finished
    document.getElementById("output").value = "Malbolge Virtual Machine v" +
        UTF8ToString(_malbolge_version()) +
        "\nCopyright Cam Mannett 2020";

    // Increase the log level
    _malbolge_set_log_level(2);

    programRunning(false);
}

/** Emscripten callback for Malbolge for handling std::clog.
 * 
 * @param {string} msg Malbolge debug output
 */
function stdClog(msg) {
    // Use regex to determine if this is Malbolge logging or Emscripten
    const res = msg.match(/\.\d{3}\[[A-Z ]+\]: /g);
    if (res) {
        consoleWrite(msg, LogSource.malbolgeWasm);
    } else {
        // Emscripten output, always bad
        consoleWrite(msg, LogSource.emscripten);
    }
}

/** Write log messages to their appropriate places.
 * 
 * @a logSrc determines where the log messages are written to.  All logging
 * sources write to the browser's console, but only Malbolge logging appears in
 * the Console pane in the page.
 * @param {string} msg Console output string
 * @param {LogSource} logSrc Logging source
 * @param {logLevel} logLevel Logging level, only used when logSrc is
 * LogSource.malbolgeJS
 */
function consoleWrite(msg, logSrc, logLevel) {
    let consoleDiv = document.getElementById("console");

    switch (logSrc) {
        case LogSource.malbolgeWasm:
            let origMsg = msg;
            if (msg.endsWith("\x1B[0m")) {
                msg = msg.substr(0, msg.length - 4);
            }

            // Logging data from the WASM side uses terminal colour hints to
            // colour the text, we will use the same
            let isError = false;
            if (msg.startsWith("\x1B[31m")) {
                msg = "<span style=\"color:darkred\">" + msg.substr(5) + "</span>";
                isError = true;
            } else if (msg.startsWith("\x1B[32m")) {
                msg = "<span style=\"color:limegreen\">" + msg.substr(5) + "</span>";
            } else if (msg.startsWith("\x1B[33m")) {
                msg = "<span style=\"color:darkgoldenrod\">" + msg.substr(5) + "</span>";
            } else if (msg.startsWith("\x1B[34m")) {
                msg = "<span style=\"color:darkblue\">" + msg.substr(5) + "</span>";
            } else if (msg.startsWith("\x1B[0m")) {
                msg = msg.substr(4);
            }

            consoleDiv.innerHTML += "<p>" + msg + "\n</p>";
            if (isError) {
                console.error(origMsg);
            } else {
                console.log(origMsg);
            }
            break;
        case LogSource.malbolgeJS:
            switch (logLevel) {
                case JSLogLevel.info:
                    consoleDiv.innerHTML += "<p>" + msg + "\n</p>";
                    console.log(msg);
                    break;
                case JSLogLevel.error:
                    consoleDiv.innerHTML += "<p><span style=\"color:red\">" + msg +
                        "\n</span></p>";
                    console.error(msg);
                break;
            }
            consoleDiv.scrollTop = consoleDiv.scrollHeight - consoleDiv.clientHeight;
            break;
        case LogSource.emscripten:
            console.warn(msg);
            break;
    }
}

/** Clears the "console" element.
 */
function clearConsole() {
    document.getElementById("console").innerHTML = "";
}

/** Sets the debug level using the level provided by the "debug_level" element.
 */
function setDebugLevel() {
    let level = document.getElementById("debug_level").value;
    _malbolge_set_log_level(level)
}

function loadPreset() {
    let pane = "";
    switch (document.getElementById("preset").value) {
        case "0":
            pane = "";
            break;
        case "1":
            pane = "('&%:9]!~}|z2Vxwv-,POqponl$Hjig%eB@@>}=<M:9wv6WsU2T|nm-,jcL(I&%$#\"`CB]V?Tx<uVtT`Rpo3NlF.Jh++FdbCBA@?]!~|4XzyTT43Qsqq(Lnmkj\"Fhg${z@>"
            break;
        case "2":
            pane = `(=BA#9"=<;:3y7x54-21q/p-,+*)"!h%B0/.
~P<
<:(8&
66#"!~}|{zyxwvu
gJ%`;
            break;
    }

    document.getElementById("program").value = pane;
    programTextInput();
}

var Module = {
    thisProgram: "malbolge.wasm",
    onRuntimeInitialized: finishedLoading,
    printErr: stdClog,
    malbolgeVcpu: undefined,
    malbolgeProgramNormalised: false
};
