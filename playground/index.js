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
    "info":1,       ///< Informational messages
    "emscripten":2, ///< Messages from the Emscripten runtime
    "error":3       ///< Error messages
};

/** Sets a running state.
 * 
 * This enables/disables components depending on @a is_running.
 * @param {boolean} is_running True to represent a state where the Malbolge
 * program is running
 */
function running(is_running) {
    document.getElementById("run_button").disabled = is_running;
    document.getElementById("preset").disabled = is_running;
    document.getElementById("stop_button").disabled = !is_running;
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
            running(false);
        } else if (e.data.cmd === "malbolgeWaitingForInput") {
            document.getElementById("input_button").disabled = false;
            console.log("Waiting for input");
        } else {
            standardHandlers(e);
        }
    };
}

/** Execute the program as provided by the "output" element
 */
function runMalbolge() {
    running(true);
    document.getElementById("output").value = "";
    const programText = new CString(document.getElementById("program").value);
    if (programText.pointer == null) {
        consoleWrite("Failed to allocate program source heap",
                     LogLevel.error);
        running(false);
        return;
    }

    // Load and validate the program
    let vmem = _malbolge_load_program(programText.pointer,
                                      programText.size,
                                      0,
                                      0);
    programText.destroy();

    if (!vmem) {
        running(false);
        return;
    }

    // Now execute it
    let vcpu = _malbolge_vcpu_run_wasm(vmem);
    if (!vcpu) {
        running(false);
        return;
    }

    wrapWorkerHandler();

    Module.malbolgeVmem = vmem;
    Module.malbolgeVcpu = vcpu;
}

/** Stop the Malbolge program execution.
 */
function stopMalbolge() {
    running(false);
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

    document.getElementById("run_button").disabled = false;
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
};
