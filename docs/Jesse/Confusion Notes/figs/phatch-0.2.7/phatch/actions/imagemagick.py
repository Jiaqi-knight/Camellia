# Phatch - Photo Batch Processor
# Copyright (C) 2007-2008 www.stani.be
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see http://www.gnu.org/licenses/
#
# Phatch recommends SPE (http://pythonide.stani.be) for editing python files.

# Embedded icon is from Imagemagick (http://www.imagemagick.org).

# Follows PEP8

from core import ct, models
from lib.reverse_translation import _t

COMMANDS = {

_t('3D Edge'):
    """%(convert)s file_in.tif  -fx A  +matte -blur 0x6  -shade 110x30  \
    -normalize \
    file_in.tif  -compose Overlay -composite \
    file_in.tif  -matte  -compose Dst_In  -composite \
    file_out.png""",

#http://www.imagemagick.org/Usage/convolve/
_t('Blur'): \
    """%(convert)s file_in.tif -blur %(blur_radius)sx%(blur_sigma)s \
    file_out.png""",

_t('Bullet'): r"""%(convert)s file_in.tif -matte \
    \( +clone -channel A -separate +channel -negate \
   -bordercolor black -border %(border)s  -blur 0x%(blur1)s \
   -shade %(shade)s \
   -normalize -blur 0x%(blur2)s -fill '%(color)s' -tint 100 \) \
    -gravity center -compose Atop -composite \
    file_out.png""",

_t('Charcoal'): \
    """%(convert)s file_in.tif -charcoal %(charcoal_radius)s file_out.png""",

_t('Motion Blur'): \
    """%(convert)s file_in.tif \
    -motion-blur %(blur_radius)sx%(blur_sigma)s+%(blur_angle)s file_out.png""",

_t('Pencil Sketch'): \
    """%(convert)s file_in.tif -colorspace gray -sketch \
    %(sketch_radius)sx%(sketch_sigma)s+%(sketch_angle)s file_out.png""",

_t('Paint'): \
    """%(convert)s file_in.tif -paint %(paint_radius)s file_out.png""",

_t('Polaroid'):
    """%(convert)s %(caption_cli)s file_in.tif \
    -bordercolor '%(border_color)s' \
    -background '%(shadow_color)s' +polaroid file_out.png""",

#http://www.imagemagick.org/Usage/convolve/
_t('Shadow'):
    r"""%(convert)s file_in.tif \( +clone  -background '%(shadow_color)s' \
    -shadow \
    %(blur_radius)sx%(blur_sigma)s+%(horizontal_offset)s+%(vertical_offset)s \
    \) +swap -background none   -layers merge  +repage file_out.png""",

_t('Sharpen'): \
    """%(convert)s file_in.tif -sharpen %(sharpen_radius)sx%(sharpen_sigma)s \
    file_out.png""",

_t('Sigmoidal Contrast'):\
    """%(convert)s file_in.tif  \
    -sigmoidal-contrast %(contrast_factor)s,%(contrast_treshold)s%% \
    file_out.png""",

_t('Unsharp'): \
    """%(convert)s file_in.tif -unsharp %(unsharp_radius)sx%(unsharp_sigma)s \
    file_out.png""",

_t('Wave'): \
    """%(convert)s file_in.tif -wave %(wave_height)sx%(wave_length)s \
    file_out.png""",
}

ACTIONS = COMMANDS.keys()
ACTIONS.sort()


class Action(models.Action):
    """Defined variables: <filename> <type> <folder> <width> <height>"""

    label = _t('Imagemagick')
    author = 'Stani'
    email = 'spe.stani.be@gmail.com'
    version = '0.1'
    tags = [_t('filter'), _t('plugin')]
    tags_hidden = ACTIONS
    __doc__ = _t('Blur, Polaroid, Shadow, Unsharp...')

    def init(self):
        self.find_exe('convert', 'Imagemagick')

    def interface(self, fields):
        fields[_t('Action')] = self.ChoiceField('Polaroid',
            choices=ACTIONS)
        fields[_t('Horizontal Offset')] = self.PixelField('2%',
                                        choices=self.SMALL_PIXELS)
        fields[_t('Vertical Offset')] = self.PixelField('2%',
                                        choices=self.SMALL_PIXELS)
        fields[_t('Color')] = self.ColorField('#FF0000')
        fields[_t('Border Color')] = self.ColorField('#FFFFFF')
        fields[_t('Shadow Color')] = self.ColorField('#000000')
        fields[_t('Caption')] = self.CharField(choices=self.STAMPS)
        fields[_t('Charcoal Radius')] = self.PixelField('0.5%')
        fields[_t('Contrast Factor')] = self.SliderField(100, 0, 100)
        fields[_t('Contrast Treshold')] = self.SliderField(50, 0, 100)
        fields[_t('Blur Radius')] = self.PixelField('80px')
        fields[_t('Blur Sigma')] = self.PixelField('3px')
        fields[_t('Blur Angle')] = self.SliderField(120, 0, 359)
        fields[_t('Paint Radius')] = self.PixelField('0.5%')
        fields[_t('Sharpen Radius')] = self.PixelField('0px')
        fields[_t('Sharpen Sigma')] = self.PixelField('3px')
        fields[_t('Sketch Radius')] = self.PixelField('0px')
        fields[_t('Sketch Sigma')] = self.PixelField('20px')
        fields[_t('Sketch Angle')] = self.SliderField(120, 0, 359)
        fields[_t('Unsharp Radius')] = self.PixelField('0px')
        fields[_t('Unsharp Sigma')] = self.PixelField('3px')
        fields[_t('Wave Height')] = self.PixelField('0px')
        fields[_t('Wave Length')] = self.PixelField('3px')

    def get_relevant_field_labels(self):
        """If this method is present, Phatch will only show relevant
        fields.

:returns: list of the field labels which are relevant
:rtype: list of strings

        .. note::

            It is very important that the list of labels has EXACTLY
            the same order as defined in the interface method.
        """
        relevant = ['Action']
        action = self.get_field_string('Action')
        if action == 'Blur':
            relevant.extend(['Blur Radius', 'Blur Sigma'])
        elif action == 'Bullet':
            relevant.extend(['Color'])
        elif action == 'Charcoal':
            relevant.extend(['Charcoal Radius'])
        if action == 'Motion Blur':
            relevant.extend(['Blur Radius', 'Blur Sigma', 'Blur Angle'])
        elif action == 'Paint':
            relevant.extend(['Paint Radius'])
        elif action == 'Polaroid':
            relevant.extend(['Border Color', 'Shadow Color', 'Caption'])
        elif action == 'Shadow':
            relevant.extend(['Horizontal Offset', 'Vertical Offset',
                'Shadow Color', 'Blur Radius', 'Blur Sigma'])
        elif action == 'Sharpen':
            relevant.extend(['Sharpen Radius', 'Sharpen Sigma'])
        elif action == 'Pencil Sketch':
            relevant.extend(['Sketch Radius', 'Sketch Sigma',
                'Sketch Angle'])
        elif action == 'Sigmoidal Contrast':
            relevant.extend(['Contrast Factor', 'Contrast Treshold'])
        elif action == 'Unsharp':
            relevant.extend(['Unsharp Radius', 'Unsharp Sigma'])
        elif action == 'Wave':
            relevant.extend(['Wave Height', 'Wave Length'])
        return relevant

    def apply(self, photo, setting, cache):
        info = photo.info
        action = self.get_field('Action', info)

        w, h = info['size']
        dia = (w + h) / 2
        values = self.values(info, pixel_fields={
            'Horizontal Offset': w,
            'Vertical Offset': h,
            'Blur Radius': dia,
            'Blur Sigma': dia,
            'Charcoal Radius': dia,
            'Paint Radius': dia,
            'Sharpen Radius': dia,
            'Sharpen Sigma': dia,
            'Sketch Radius': dia,
            'Sketch Sigma': dia,
            'Unsharp Radius': dia,
            'Unsharp Sigma': dia,
            'Wave Height': dia,
            'Wave Length': dia,
        })
        values['convert'] = self.exe['convert']
        if action == 'Bullet':
            #extra values
            values['border'] = dia / 3
            values['blur1'] = dia / 7
            values['shade'] = '%dx%d' % (dia + 105, dia + 15)
            values['blur2'] = dia / 12
        elif action == 'Polaroid':
            caption = values['caption']
            if caption.strip():
                values['caption_cli'] = \
                    '-caption "%s" -gravity center' % caption
        elif action == 'Sigmoidal Contrast':
            values['contrast_factor'] /= 10.0

        command = COMMANDS[action] % values
        #print(command)  #use "python phatch.py -v" for verbose mode
        photo.call(command)
        return photo

    icon = \
'x\xda\x01\x03\x0e\xfc\xf1\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x000\
\x00\x00\x000\x08\x06\x00\x00\x00W\x02\xf9\x87\x00\x00\x00\x04sBIT\x08\x08\
\x08\x08|\x08d\x88\x00\x00\r\xbaIDATh\x81\xd5\x9a{\x90T\xd5\x9d\xc7?\xe7\xde\
\xdb\xaf\xa1a\xde\xcc\x003 \x0fet$\xf2R\x03\x0b\x1b5\xe0\x82\xb2\xac\xa2"\
\x11\'*\xbb\xb5[&\xf8(wI\xb2\xea\xban\xca\x07\xba\xee\xd6\x96\x86\x8a\xa1\
\xccJ\\A\xa3\x1b\x93\x94\xd1\x10I\x16P2+\xf3l\x06\xa7\x07\xe413\xd0\xc00=/\
\xe7\xd1\xd3\xdd\xf7\xf1\xdb?\xba\xa7\x99\x9ei\x10Pk+\xbf\xaaS}\xe7\xcc\xbd\
\xe7|?\xbf\xdf\xef<\xee\xe9V"\xc2\x9f\xb2i\xa3j\x02\xeag\x04\xd4\xfc\xaf\xa2\
\xb3\xda\xda\xda\xac\x8f?\xfe\xf8\xce/\xb3M#C\xdd\x8d\xc0Z\x02\xea%\xe0\x87D\
\x98I\x84EDXH\x84\n\xd6\xc8\xc0\xc5tTSS\xb3\xd2\xb6\xed\x17\x1d\xc7i\x01~\
\xfe\x054\xa7Y:@@)\xa0\x00\xd01y\x98\x08\x0f\x13\x81d\xd9G\xc5\x85\x8b\xaf\
\xaf\xaf\x9f""/\xda\xb6\xbd\xd2\xb2,l\xdbn\x01\xd8\xb3gOn___d\xf9\xf2\xe5\
\xb1/\x0f\x00\x8a\x00\x83\xc1\x94\xe8D\x19\x00"\xec\xbc\x90\x86kkk]n\xb7\xfb\
\x11\x11y\xc2\xb2\xac,\xcb\xb2hii\xc1\xe3\xf1\xb8jjj*t]\x7f\xc1\xe7\xf3]\x05\
\xb4}9\x00\x015\x05\x8b\xdf\xa4\tO/\x1f\x9eo\xa3\xc1`p\xb1\xdb\xed\xfe\xb1eY\
\xe5\xb6ms\xea\xd4)\xea\xea\xea\xb0,\x8be\xcb\x96]iY\xd6k\xb6m\x7f\x11\xdd#\
\x00\x02j\x01\x83\xfc\x8a\x08\xe3\xcf\x01\xb0\x02\xf8\xe5\xb9\x1a\xab\xaf\
\xaf/t\xbb\xdd\xcf;\x8es\xafeYtvvRWW\xc7\x89\x13\'\x98={6\xe5\xe5\xe5\x88\
\xc8\xb8x<\xce\xb1c\xc7\xc8\xcf\xcfW_\x14@\x89\x08|\xa4\x86\xf2}\xca9\x00\
\x84\x01\x16\xf1\x9aTfj\xa7\xb1\xb1\xf1o\x80\x8d"\x92\x17\x8dF\t\x04\x024662\
q\xe2D\x16,X\x80\xcf\xe7\xc34MZZZ\xa8\xaf\xafGD\x98;w\xee$\xdb\xb65\xcb\xb2\
\xae\x02.\xb9\xe7\x9e{6]\x1c\x00\xc0[J#\xc2\xcdD\xf8\'"\\\x9d\x11\x02\xf6\
\x01\xf3\xd8&\xa9\xf8766^\xa5\x94\xfa\xb1\x88,\x00\x08\x85BTVVb\x9a&__x\r\
\x97L\x9e\x86eY\x9c<y\x92\xda\xdaZ\xda\xdb\xdb\xc9\xca\xcab\xd2\xa4I\x14\x16\
\x16\x9e\x16\x91\xa2\xa4\x86\xf7***V\\(\xc0\x991\xb0Z\x1c\xe0]\x9eU\xc7\x89P\
\x9f\x14\xed\x10\xe1\xfb\x984\x01\x9d\xc9\xa2\x00\x82\xc1\xa0_D~\xa8\x94zPD\
\xf4X,\xc6\xde\xbd{9|\xf80\x97_q9\xf3\xe6\xcdC\xd7t\xfa\xfa\xfa\xa8\xaa\xaa\
\xa2\xb9\xb9\x99\x9c\x9c\x1c\xe6\xcf\x9f\xcf\x981c\x10\x11\x86\xc4\x8b\x08\
\x8e\xe3\xec\xbaP\xf1\xe9\x00C\xf6\x8f\x12\xe0!U\x9d\x8c\xc2\xc3l\x95\x97F\
\xde\x12\x0c\x06o\x07\xfe\x03\x98\xe48\x0eG\x8e\x1c\xa1\xba\xba\x9a\xb1c\xc7\
\xf2\x97\xb7\xdcLa^\x11\xb6ms\xf0\xe0Ajjj\xd04\x8d+\xaf\xba\x82\xf1\xf9\xc58\
\x8e3$\x9ea\xe2\xd1u}\xf7(-\xb5\xaa\x0c\x9d\xb9\xcc\x96m\xe7\x0f\x00\xf0\x19\
\x9b\x89\xb3\x87m\xe9\xe2\xf7\xef\xdf?M\xd3\xb4\x1f\x01\xcb\x01\xba\xbb\xbb\
\xa9\xaa\xaa\xa2\xb3\xb3\x93y\xf3\xe7p\xd9\xa5e(\xa5\xe8\xe9\xe9\xa1\xb2\xb2\
\x92\xf6\xf6v\xa6M\x9fJi\xc9d\x00D\x04\xa5\xd4(\x00\xa5T?P7J\x87\xce#\xc0Z\
\xeaU#sd_&\xa9*\xe3^\xe8.\xe5\x01L\xb6\x89\x93\xf4\xb8[D\xbe\x07<\x06x-\xcb\
\xa2\xa1\xa1\x81`0\xc8%\x97\\\xc2\xdc\xf9s\xc8\xf2\x8e\xc1\xb6m\xf6\xef\xdfO\
CC\x03\x13\'Nd\xda\x8c\xa9\xb8\rO\x9a\xa7E\x84\x98-\x1c\xeeu(\x1b\xc7P]\xd5\
\xaaU\xab\xaeM\xd3P\xaf\n\xd18\x86\xe0\x05\x8e\xa23\x9fY\xd2}~\x11\xd8&\xa9\
\xd51\x18\x0c~\x13\xd8\x04\xcc\x14\x11B\xa1\x10UUU\x18\x86\xc17\x97^\xcf\x84\
\xa2I\x00\x9c8q\x82\xaa\xaa*\x94R,Xx-\xe3\xc6\xe6\x8cJ\x93A\x1b\x02\x9d\x16\
\xfb\xbb\x1d\xa6\x8d\xd5(\xd7\xd5PD\x0e\xa6\xf5\xff\x89*B\xb1\x11\x07/&`1\r\
\x93C\xfcA\x1d\xc1\xe4\x04&\xad\x18\xfc\x80\xe5\x12\xcb\x0c\x90\x10^,"\xff\
\x06\xdc\x05\xd0\xdf\xdfOuu5\'O\x9e\xe4k\xb3gQ~\xc5\x95hJ#\x12\x89\xa4\xea/\
\xbf\xbc\x8c\xc9\x93\xa7\x8cJ\x91A\xcb\xa1\xf2\xb4\xc9\xfe.\x0b\xcb\x11\\\
\xba\xc6\xcc\x1c\x17\x9a\xe6\xa0\x94\xc2q\x9c\x83\xa04\xea\xb9\x15\x8b{\xb1X\
\x86\x89\x81\x05I\x000\xc9\xc7$?y\xbd\x91o\'\x9c\x9c\x11\xa0\xb6\xb66\xcb\
\xe7\xf3\x1d\x00\xb2\x1d\xc7\xa1\xa9\xa9\x89@ \xc0\x84\t\x13Xy\xcb\n\xc6\x8e\
\xc9\xc6q\x1c\x82MA\xf6\xed\xdb\xc7\xa4I\x93\xb8\xf1/\x96\xe2vy\xd2\x06\xa9#\
BC8\xc6\xce\xe3\x11\xa2\xb6`h\x8a,\xb7\x81\xc7\xd0)\xf5\xebh\xa8\xa11p\x00\
\xc4\xa1[]\x8a\xc5\x8a\x11\xc2\xd3\xaf-\x0e`\xf3\xe49S\xc8\xef\xf7\xeb\x96ee\
\xb7\xb7\xb7\xb3w\xef^b\xb1\x18\x8b\xbe\xb1\x90)%S\x01\x08\x87\xc3\xec\xdd\
\xbb\x17\xcb\xb2X\xb4x!\x85\x05E\xc3\x07$"B{\xc4\xe2\xfd#}\x1c\xef3\xd15\x85\
/)\xdcm\xe8\xe4xt\xdc\x86 \x92\xb8W\xd3\xb4D\n\xdd \x1byK\x9d\xc2\xe4\x15L\
\x8c\x11\xc2\x87`\xbe\xcb\xe3gR<#@\x7f\x7f\xbf\xedv\xbb\xd9\xb5k\x17\xa5\xa5\
\xa5\\}\xcd|\x0c\xddE,\x16\xa3\xae\xae\x8e\xe6\xe6ff}m\x16eee(TZ\x9e\x9b\x0e\
\xec:\xd6\xcb\xc7\'\x06\x10\xc0\xeb2(\x88\x87)\xcfqs8>\x06\xf1f3>KC\xd7\xcfD\
\xca\xb2\xac\xe6T\xe7\xab\xe5g\xbc\xa4\xdcXl>\x0b@\xda\xac\x93\x11 ??\xdf\
\xe9\xeb\xebC)Eii)\xbafp\xf4\xe8Qjjj\x18?~<\xb7\xdc\xfaW\x8c\xc9\xf2\x8f\xca\
\xf5\x03\x9dQ\xde=\xd8Io\xdcNy\xdb\xabl*\xe6^\xca\xf8\xc2B\x1c\xc7ak\xd5Q\
\xf2}\xb9\xe8\xba\x95z\xce\xe7\xf39i\x02z80*u\x86\xae-\xe6\xc3\x99\x9dqF\x80\
H$\xe2\x00\xa9thmm\xa5\xba\xba\x9a\xeb\xae\xbf\x8e\xc9\xa5\x93G-F\x9d\x918\
\xbf\n\x86i\n\x0f\xe01t\xfc\x1ew\x02\xc0\xa5\xe3\xf4\xb4\x81\xe4\x10\x8f\xc7\
\xf1z\xbd,\x99:\x8ecq\r]\xd7S\xcf\xf7\xf5\xf5\xa5\xbf\x19v10L\xf8\x1f\xb1x\
\x1f\x93%\xc0" \xedm1#\xc0\xdbo\xbf\xed\xacZ\xb5jh\x86\x00 //\x8fiS\xa7\xe18\
N\n\xcc\xb2\x1dv\x1e\xed\xe4\x83C\x1d(\x05~\xaf+\xe5y\x8f\xa1\xd3~h?\x1d\xcd\
\x07\xf8`\xe0\x187/\xbd\x01]\xd7)\x9d4\x91S\x81\x06t\xff\x84\x14@4\x1aM\x07\
\xe8!B\x9c\xe3\xc0\xf7\xd8*o&k\x9fa\xb5\xf2c0\xeb\xac\x00J)\x05\xa8\xf2\xf2r\
m\xd5\xaaU@b\xf5\x1c\xfa\xd4u=%\xfeH\xe7\x00[\xeb\x8e\xd3\x151S\xa2\xdd.=\r\
\xe0\xd4\xc0g\xf8\xfd~\x1aO\xf5Rv\xf4(3/\xbb\x0cM\xd3\xb8\xfa\xaaY\x04\x83A\
\x8a\x8b\x8b\x11\x11"\x91\xc8HG\xb6`2\x93\xb7d0\xad\xf6-\xe9\x07\xfe7#\x80RJ\
#\xf1\x92o466z\x92u\xa9\x99\x05@\xd3\xb4T]ek7\xfdq\x07\xbf\xd7\x8d\xc7\xd0\
\x12\x00\xc60\x00\x97NNQ\x0e{\xdb\x7f\xcfQUG]]!\xffY\xfcS\\.\x17\xd1h\x14\
\x8f\xc7\x83eY\xb8\xddnb\xb1\x11\xeb\xd1\xab\x12\xcd\x94\x19\x99\xcc\x18\xf2\
z\x12\xc6\r\xf8\x80\xb1\x80#"Zr\x9aKEB\xa9\xc4\xea\x99\xe56\xf0{\\\xa3\xbc\
\xee6t\\\x86F@\x7f\x97\xf7\x8a61p\xba\x0bLh1\x07\xf8\xd1\xaeMT\xcc\xb9\x9b\
\xcd\x9b7SRR\xc2\xf4\xe9\xd3\xb9\xf1\xc6\x1b)(((\x00\xc2\xe7+zd\x04\xb4a\xc2\
\xfd\xc08 \xc7\xb6\xed\x14\xc0\x90\xd7\x87\x9b\xd7e\xe0\xf7\xbaq\x1bZ\x9a\
\xd7]\x86\xc6;\xd6\xb3Tv\xbf\x93\x18\x84Q \x9e(\xef\x84\x7fI\xdb\xabm\x1c\
\xae=\x84\xd7\xeb\xc5\xe7\xf3\xd1\xd8\xd8Hqq\xf1d\xa0\xe9b\x01\xdcI\xe19@^\
\xb2d\xc7\xe3ql\xdb\xc6\xb6m\\.Wj0\x0fY\x96\xdb\xc0\xefu\xa5\xa7\x8d\xa1\xf3\
\x8b\xbe\x8dT\x86\xdfI\x88\x1e\x04\x1c\x05\xfdI\x88~\xa8\xf6\xd4\xa0G4>\xfd\
\xf4S|>\x1f\x9f|\xf2\t"2O)\xf5\x81\\\xc4)\x9b\x01x\x81l\x12\'\x12\xe3\x81\\\
\xc0oYV\xaa=\xdb\xb6G\x01$"p\x06\xc0\xd1\xa3l\xea~\x88\x03\x91j\x18C"\x9eY\
\xc9\x9b\xdb\x80N\x98\x9f}-\x1b\xef~\x86O\xafo\xe2\x81\x07\x1e\xa0\xa0\xa0\
\x00\xaf\xd7K0\x18|\x10\xd8\x08\\\x14\x80+\xd9U\x0e\x90O"\x02Y\xb6mKr\x95\
\xc40\x0cF\x9e"x]\x89\xf9>\xae\xf7\xf2~|\x0b\x7f\xe8\xda\x86i\xc6\xd3\x17\
\x1e\x13p\xc0\xb0]\xdcs\xf5}\xdc7\xfbn\n\xc6\x150m\xd9\x14n\xbb\xed6\xb6o\
\xdfNvv6>\x9f\xafh\xdd\xbau\xcf\x03\xffp\xa1\x00Z\xa2\x0b\x84\xc4@v\'a\xc6\
\xc6\xe3q\x1c\xc7!y\x185*\x02>\xb7A\x9b\xde\xc8\xb3]k\xd8\x1e\xde\x82\xd9\
\x1fO\xa4\xca\xb0\xa2z\x157\xf8\x97\xf2\xe1\xda]\xac\xff\xb3\xbf\xc3\xef\xf7\
\xa3\x94B\xd7u\xd6\xaf_\x7f\xda4\xcd\x8e\x86\x86\x06:::\xc8\xcd\xcd}h\xed\
\xda\xb5\x8b.\x06 \xce\x99\xe3\xabx\xb2\xceg\x9afj\x7f\xe38\xce\xa8\x08\xec\
\x19x\x8f\xe7\x8f\xdfKWw\x1b\xf4\x91^z\xa14:\x85\xdf\xad\xd8\xcek\xb7\xfc\
\x94\xf1y\x85x\xbd\xde\xa1w\xe1\xe8\x8e\x1d;\xb6\xac\\\xb9r\xd9\xe0\xe0\xe0\
\xb7Z[[\xad\x96\x96\x16:::\x8c\t\x13&\xbcq\xe7\x9dw\xfa.\x04\xc0 5\xbc\xe8It\
M\x0c\xd0,\xcbJE`\x08b\xb85u\xd7c\xf7\xd9\xe9\xe9b\x82/\x9e\xc5\x13\x0b\xff\
\x99\x8a9\xdfJ\xdd\xeb\xf1x0\x0cC\x82\xc1\xe0\x9e\xe7\x9e{\xee\xd9\xdd\xbbw\
\xef\x03\xba\x80\xb8\x88<\xb9o\xdf\xbe\xa7rss\xf1\xf9|%yyy\xff\x05\xdc~\xbe\
\x00\x1a\x89\xac\x8d\x00\xdd@\x07\xf0\x19`\x9a\xa6)\x8e\xe30|6\x1ann\xd3\x95\
\xe6u\xd5\xabqk\xf1\xed\x1c\xb8\xbf1M<\xc0\xa9S\xa7Z^x\xe1\x85\xf57\xddt\xd3\
=\xbbw\xef\xfe\x088\r\xc4D\xc4\x01\x9e\x8dF\xa3{\xea\xeb\xeb\xe9\xe8\xe8\xc0\
\xe7\xf3\xad\xaa\xa8\xa8X{\xbe\x00\x06\x891\x10#\xe1\xfdp\xb2\xf1"\xd34\xc54\
Mb\xb1X\xc6\x08\xb8-\x03\xfa@\xb34.\xcb*\xe3\xd5\x8aW(\xcd-I\xbbg``\xa0o\xfb\
\xf6\xed[6l\xd8\xf0\x93x<~\x92D\xa4\xad\xe1\xd3\xa5\x888J\xa9\xbbO\x9f>\xddp\
\xf0\xe0\xc1q>\x9fO\x15\x17\x17oZ\xb3f\xcd\xce7\xdf|\xf3\xe4\xf9\x00\x08`\
\x93\x98\xb5\xbbHLz\xe3m\xdbv\x06\x07\x07\x19\x18\x18 77wt\x04\x1c\x17\x0b\
\xf2\x17\xf3\xd4\x92\x7faf\xc1\x8c\xb4\xff\xd9\xb6mWUU\xfd\xee\xe9\xa7\x9f~>\
\x10\x08\x1c \x91\x9ef\xd2\xe3\xa3LDZ\x95R\xdf\t\x06\x83\xaf\'\xa7\xd6\xec\
\xa2\xa2\xa2w\x80\xaf\x7f\x1e\x80\x96\xf4\x86Cb,\xf4\x91\x88@\xc8\xb2,\xcbq\
\x1c\'\x12\x89\xc43\xcdB\xf7\xcf\xf9k\xde^\xb3u\x94\xf8\xe6\xe6\xe6\xe0c\x8f\
=\xb6\xee\x8e;\xee\xf8N \x10\xa8\x01:D$v6\xf1\xc3 \xb6\xda\xb6\xfdFmm-\xe1p\
\x18\xe0\xdau\xeb\xd6=\xf9\xb9\x00\xc9\x87\x87 \xa2$\xc6\xc2\xf1P(T\xb5c\xc7\
\x8e\xad\xfd\xfd\xfd\xa73\x01\x14\xf9\x0b\xd3\xfe\xee\xe9\xe9\xe9\xdc\xb2e\
\xcbS\x8b\x17/\xbe\xed\xf5\xd7_\xff5p\x02\x88\x88\xc8\xd9\x8f\xa1w)\xef\x88\
\x9a\xfb{{{\x8f\rM\xad\x05\x05\x05?X\xb3f\xcd\xdc\xcf\x05H\x9a\x90\x18\xd0\
\xfd@\xdb\xa3\x8f>\xfaxkk\xeb\x8e\xac\xac\xac6\xc7qp\xb9\\\x19=h\x9a\xa6\xb9\
s\xe7\xce\xb7\x96-[\xb6\xe2\xf1\xc7\x1f\x7f\x118\n\xf4\x8a\x88u\xd6\xadA\xad\
r\x11P\xaf\x92\xcd\x1d\xc3\xabE\xe43\xa0\xa2\xb9\xb9\xd9inn\xa6\xab\xab\xcbS\
RR\xf2\xdf\xabW\xafv\x7f.\xc0\x88T\xea\x01Z\x81:\xbf\xdf\x1f\n\x85B\x07v\xee\
\xdc\xf9\xc7\x91\x0f755U\xaf_\xbf~MEE\xc5\xdf\x87B\xa1\x06\xa0SD\xe2\xa3\x84\
\xd7\xab+hP3\xa9U\x05T\xa9|\x0c~\x0b\xdc\x8b\xe2\xcfG\xb6)"\x1f\x8a\xc8s\xf5\
\xf5\xf5\xb4\xb5\xb5\x11\x8dF\xa7\xe6\xe6\xe6\xber6\x80Q\xef\xb5$Vd\x1d\xf0\
\x00\xd9\xe5\xe5\xe5\xd7\x00\xdf\x9d1c\xc6\xcfC\xa1\x90\x84B!\xa9\xab\xab;\
\xb1a\xc3\x86\r\xc0\x0c\x12\xbbW\x83\xe4)_\xc6\xf2{\x9e\x90\xdd\x88\xecA\xe4\
cD\xaa\x11\xa9C\xa4\x9eO3\xddOb{SS\\\\,K\x96,\x91\xeb\xae\xbb\xceY\xbat\xe9}\
\x19\xef=K\x03\xc3\xdf\x11\xc6\x03\xcb\xcb\xca\xca\xfe\xf5\xd0\xa1C\x03/\xbf\
\xfc\xf2+c\xc6\x8c\x99Mb\xdf\xe4:\xa7\xf0\xa1\xb2\x8d"\xf9\x051\xf95"\xbfA\
\xe4\xb7\x88\xec@\xe4\x7f\x10\xf9\x90\xcdRI^\x06\r3\x81\x88RJ\x00)))\xe9\xcb\
\xd4v\xe6\xb3\xd1a\xa6\x94\xf2\x02%\x85\x85\x85\xd3\xcb\xca\xca\xac\x8f>\xfa\
\xa8\x893\xab\xe89g\x964{EmE\xe3.\x8cd|\xf5\xa4{\x12\x9f\x9d\x18<\xc2rymD\
\xdf\x8f\x02O\x03\xe4\xe5\xe5\xf5tvv\xe6\x8el\xf6\xacG\x8b\xc3,\x0e\x84\xc2\
\xe1pG8\x1c\x8e\x93X\xf4\x9c\x0b\xde\xbb\x87\xc9\x1b&x$\xc4\'\xb8\xa9\x19\
\xf9\x88\x88<\xa3i\x9a\x8c\x1d;\xf6V\xb7\xdb\x9dq\xa7\xfa\xb9\x11\x80\xd4\
\xcb\xfeP\xa3\x17\xfe\xd5\xfe\x83j:n\x0e\xa1\xa3F@\x08\x06\xf7\xf2\xfdt\xcf_\
\x88\x9dO\x04.N\xf4p\xeb\xe1~4"\x18\x84\xd1\x89cpY\x12\xa2\x89\x7f\xbfx\xf1C\
\xe2\xbe\xfa\xf2\xb7\xb8R\xd7\xa0d\rO\xcb]\x88T\xf0\x93/\xda\xf6y\xa5\xd0Wbk\
\xd5\xb7q\x88\xf1\x86|\xa1\x9f\x1d\xfc\xff\x01|I6\xfa\xd7*\x7fb\xf6\x7f\xef{\
~\xe5\x17+\xc5\x93\x00\x00\x00\x00IEND\xaeB`\x82\x04R\x04\xfc'
