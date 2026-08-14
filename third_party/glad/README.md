# GLAD loader

The files in this directory were reproducibly generated with GLAD 2.0.8 for the OpenGL 3.3 core profile, with no optional extensions:

```text
glad --api gl:core=3.3 --extensions <empty-file> --reproducible c
```

They are committed so consumers do not need Python, Jinja2, or network access to run GLAD's generator during every build. See the notices at the top of the generated files and the [GLAD repository](https://github.com/Dav1dde/glad) for source and licensing details.
