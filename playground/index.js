/* Cam Mannett 2020
 *
 * See LICENSE file
 */

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

/** Log levels.
 */
const LogLevel = {
    "info": 1,       ///< Informational messages
    "emscripten": 2, ///< Messages from the Emscripten runtime
    "error": 3       ///< Error messages
};

/** Sets a running state.
 * 
 * This enables/disables components depending on @a isRunning.
 * @param {boolean} isRunning True to represent a state where the Malbolge
 * program is running
 */
function programRunning(isRunning) {
    if (isRunning) {
        document.getElementById("program_button").innerText = "Stop";
    } else {
        if (Module.malbolgeProgramNormalised) {
            document.getElementById("program_button").innerText = "Run Normalised";
        } else {
            document.getElementById("program_button").innerText = "Run";
        }
    }

    document.getElementById("program_button").value = isRunning ? "stop" : "run";
    document.getElementById("preset").disabled = isRunning;
    document.getElementById("program").disabled = isRunning;
}

/** Returns true if the program is running.
 * 
 * @return True if running
 */
function isProgramRunning() {
    return document.getElementById("program_button").innerText == "Stop";
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
        if (e.data.cmd === "malbolgeStopped") {
            programRunning(false);
        } else if (e.data.cmd === "malbolgeWaitingForInput") {
            document.getElementById("input_button").disabled = false;
            console.log("Waiting for input");
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
    document.getElementById("program_button").disabled = isEmpty;

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
    document.getElementById("program_button").innerText = normalised ? "Run Normalised" : "Run";
}

/** Normalies or denormalises the program source depending on the corresponding
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
        let newSizePtr = _malloc(8); // Size is 64bit
        setValue(newSizePtr, 0, "i64");

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
            LogLevel.error);
        programRunning(false);
        return;
    }

    // Load and validate the program
    let vmem = null;
    if (Module.malbolgeProgramNormalised) {
        vmem = _malbolge_load_normalised_program(programText.pointer,
            programText.size,
            0,
            0);
    } else {
        vmem = _malbolge_load_program(programText.pointer,
            programText.size,
            0,
            0);
    }
    programText.destroy();

    if (!vmem) {
        programRunning(false);
        return;
    }

    // Now execute it
    let vcpu = _malbolge_vcpu_run_wasm(vmem);
    if (!vcpu) {
        programRunning(false);
        return;
    }

    wrapWorkerHandler();

    Module.malbolgeVmem = vmem;
    Module.malbolgeVcpu = vcpu;
}

/** Stop the Malbolge program execution.
 */
function stopMalbolge() {
    programRunning(false);
    document.getElementById("input_button").disabled = true;
    _malbolge_vcpu_stop(Module.malbolgeVcpu);
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
                LogLevel.error);
            return;
        }

        _malbolge_vcpu_input(Module.malbolgeVcpu, ctext.pointer, ctext.size);
        ctext.destroy();

        document.getElementById("input").value = "";
        document.getElementById("input_button").disabled = true;
    }
}

/** Callback from the Emscripten runtime to notify that the WASM has finished
 * loading.
 */
function finishedLoading() {
    // Print the version once the WASM initialisation has finished
    stdOut("Malbolge Virtual Machine v" +
        UTF8ToString(_malbolge_version()) +
        "\nCopyright Cam Mannett 2020");

    // Increase the log level
    _malbolge_set_log_level(2);

    programRunning(false);
}

/** Emscripten callback for Malbolge for handling std::cout.
 * 
 * @param {string} msg Program output
 */
function stdOut(msg) {
    document.getElementById("output").value += msg + "\n";
}

/** Emscripten callback for Malbolge for handling std::clog.
 * 
 * @param {string} msg Malbolge debug output
 */
function stdClog(msg) {
    // Use regex to determine if this is Malbolge logging or Emscripten
    const res = msg.match(/\.\d{3}\[[A-Z ]+\]: /g);
    if (res) {
        // It is Malbolge logging, so check if it is an error
        if (msg.includes("[ERROR]: ")) {
            consoleWrite(msg, LogLevel.error);
        } else {
            consoleWrite(msg, LogLevel.info);
        }
    } else {
        // Emscripten output, always bad
        consoleWrite(msg, LogLevel.emscripten);
    }
}

/** Write to the console.
 * 
 * Output is written to the browser's console as well the "console" element.
 * @param {string} msg Console output string
 * @param {logLevel} logLevel Logging level
 */
function consoleWrite(msg, logLevel) {
    let consoleDiv = document.getElementById("console");

    switch (logLevel) {
        case LogLevel.info:
            consoleDiv.innerHTML += "<p>" + msg + "\n</p>";
            console.log(msg);
            break;
        case LogLevel.emscripten:
            consoleDiv.innerHTML += "<p><span style=\"color:orange\">" + msg +
                "\n</span></p>";
            console.warn(msg);
            break;
        case LogLevel.error:
            consoleDiv.innerHTML += "<p><span style=\"color:red\">" + msg +
                "\n</span></p>";
            console.error(msg);
    }

    consoleDiv.scrollTop = consoleDiv.scrollHeight - consoleDiv.clientHeight;
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
    print: stdOut,
    printErr: stdClog,
    malbolgeVmem: undefined,
    malbolgeVcpu: undefined,
    malbolgeStoppedCb: undefined,
    malbolgeWaitingCb: undefined,
    malbolgeProgramNormalised: false
};
