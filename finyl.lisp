(in-package :finyl)

(defparameter *sample-rate* 44100)
(defparameter *period-size* 64)
(defparameter *dphase* (/ (* 2 pi 440) 44100))
(defconstant +2pi+ (* 2 pi))
(defparameter *phase* 0)

(defun alsa-setup (handle& buffer-size& period-size&)
  (cffi:with-foreign-object (hw-params&& :pointer) ; pointer to pointer
    (symbol-macrolet ((hw-params& (cffi:mem-ref hw-params&& :pointer)))
      (snd-pcm-hw-params-malloc hw-params&&)
      (snd-pcm-hw-params-any handle& hw-params&)
      (snd-pcm-hw-params-set-access handle& hw-params& :snd_pcm_access_rw_interleaved)
      (snd-pcm-hw-params-set-format handle& hw-params& 2)
      (snd-pcm-hw-params-set-rate handle& hw-params& *sample-rate* 0)
      (snd-pcm-hw-params-set-channels handle& hw-params& 2)
      (snd-pcm-hw-params-set-period-size-near handle& hw-params& period-size& (cffi:null-pointer))
      (snd-pcm-hw-params-set-buffer-size-near handle& hw-params& buffer-size&)
      (snd-pcm-hw-params handle& hw-params&)
      (snd-pcm-get-params handle& buffer-size& period-size&)
      (snd-pcm-hw-params-free hw-params&))))


(defun fill-frame (buffer& i)
  (let ((sample (round (* 32767 (sin *phase*)))))
    (setf (cffi:mem-aref buffer& :short (* 2 i))
          sample)
    (setf (cffi:mem-aref buffer& :short (1+ (* i 2)))
          sample)
    )
  (incf *phase* *dphase*)
  (if (>= *phase* +2pi+)
      (decf *phase* +2pi+))
  )

(defun zero-buffer (buffer&)
  (loop for i below (* 2 *period-size*) do
    (setf (cffi:mem-aref buffer& :short i) 0))
  )

(defun fill-buffer (buffer&)
  (zero-buffer buffer&)
  
  )

(defun alsa-loop (handle&&)
  (loop
    with buffer& = (cffi:foreign-alloc :short :count (* 2 *period-size*))
    do
       (fill-buffer buffer&)
       (let ((err (snd-pcm-writei (cffi:mem-ref handle&& :pointer) buffer& *period-size*)))
         (if (= err -32)
             (progn
               ;; (princ "underrun")
               (snd-pcm-prepare (cffi:mem-ref handle&& :pointer)))
             (when (< err 0)
               (snd-pcm-recover (cffi:mem-ref handle&& :pointer) err 0)
               (error (format nil "Serious error: ~A" (snd-strerror err))))))
    ))


(defparameter handle-ref nil)

(defun alsa-start (device)
  (let ((err 0))
    (cffi:with-foreign-object (handle&& :pointer)
      (setf handle-ref handle&&)
      (unwind-protect
           (progn
             (setf err (snd-pcm-open handle&&
                                     device
                                     0
                                     0))
             (when (< err 0)
               (error (snd-strerror err)))
             
             (cffi:with-foreign-objects ((buffer-size& :int)
                                         (period-size& :int))
               (setf (cffi:mem-ref period-size& :int) *period-size*)
               (setf (cffi:mem-ref buffer-size& :int) (* *period-size* 2))
               (format t "want: ~A and ~A~%" (cffi:mem-ref buffer-size& :int) (cffi:mem-ref period-size& :int))
               (alsa-setup (cffi:mem-ref handle&& :pointer) buffer-size& period-size&)
               (setf *period-size* (cffi:mem-ref period-size& :int))

               
               (format t "got: ~A and ~A~%" (cffi:mem-ref buffer-size& :int) *period-size*))

             (alsa-loop handle&&)
             )
        
        (format t "closing")
        (unwind-protect 
             (setf err (snd-pcm-drain (cffi:mem-ref handle&& :pointer)))
          (format  t "tried"))
        (when (< err 0)
          (error (format nil "drain failed: ~A" (snd-strerror err))))
        (snd-pcm-close (cffi:mem-ref handle&& :pointer))
        (format t "closed")
        )
      )))
