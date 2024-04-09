(pushnew (uiop:getcwd) ql:*local-project-directories*)
(ql:quickload :finyl)
(asdf:load-system :finyl)
