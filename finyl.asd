(asdf:defsystem :finyl
  :name "finyl"
  :author "0xsuk"
  :depends-on (:cffi)
  :serial t
  :components (
               (:file "package")
               (:file "cffi")
               (:file "finyl"))
  )
