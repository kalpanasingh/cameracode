cdef extern from "capture.h":
    ctypedef struct captureReturn:
        int errorFlag
        int captureTime
        int activeCams
    captureReturn capture(char* dirName)


def py_capture(dirName):
    cdef bytes py_bytes = dirName.encode()
    cdef char* c_string = py_bytes
    ret=capture(c_string)
    return ret.errorFlag,ret.captureTime,ret.activeCams
    