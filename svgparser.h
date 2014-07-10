#include "SkPoint.h"

namespace SvgParser {
    
    bool isNumeric( char c )
    {
        return ( c >= '0' && c <= '9' ) || c == '.' || c == '-' || c == 'e' || c == 'E' || c == '+';
    }
    
    float parseFloat( const char **sInOut )
    {
        char temp[256];
        size_t i = 0;
        const char *s = *sInOut;
        while( *s && (isspace(*s) || *s == ',') )
            s++;
        if( ! s )
            throw std::exception();
        if( isNumeric( *s ) ) {
            while( *s == '-' || *s == '+' ) {
                if( i < sizeof(temp) )
                    temp[i++] = *s;
                s++;
            }
            bool parsingExponent = false;
            bool seenDecimal = false;
            while( *s && ( parsingExponent || (*s != '-' && *s != '+')) && isNumeric(*s) ) {
                if( *s == '.' && seenDecimal )
                    break;
                else if( *s == '.' )
                    seenDecimal = true;
                if( i < sizeof(temp) )
                    temp[i++] = *s;
                if( *s == 'e' || *s == 'E' )
                    parsingExponent = true;
                else
                    parsingExponent = false;
                s++;
            }
            temp[i] = 0;
            float result = (float)atof( temp );
            *sInOut = s;
            return result;
        }
        else
            throw std::exception();
    }
    
    void ellipticalArc( SkPath &path, float x1, float y1, float x2, float y2, float rx, float ry, float xAxisRotation, bool largeArcFlag, bool sweepFlag )
    {
        // This is a translation of the  spec section "Elliptical Arc Implementation Notes"
        // http://www.w3.org/TR//implnote.html#ArcImplementationNotes
        float cosXAxisRotation = cosf( xAxisRotation );
        float sinXAxisRotation = sinf( xAxisRotation );
        const SkPoint cPrime = SkPoint::Make( cosXAxisRotation * (x2 - x1) * 0.5f + sinXAxisRotation * (y2 - y1) * 0.5f, -sinXAxisRotation * (x2 - x1) * 0.5f + cosXAxisRotation * (y2 - y1) * 0.5f );
        
        // http://www.w3.org/TR//implnote.html#ArcCorrectionOutOfRangeRadii
        float radiiScale = (cPrime.x() * cPrime.x()) / ( rx * rx ) + (cPrime.y() * cPrime.y()) / ( ry * ry );
        if( radiiScale > 1 ) {
            radiiScale = sqrtf( radiiScale );
            rx *= radiiScale;
            ry *= radiiScale;
        }
        
        SkPoint point1 = SkPoint::Make( (cosXAxisRotation * x1 + sinXAxisRotation * y1) * (1.0f / rx), (-sinXAxisRotation * x1 + cosXAxisRotation * y1) * (1.0f / ry) );
        SkPoint point2 = SkPoint::Make( (cosXAxisRotation * x2 + sinXAxisRotation * y2) * (1.0f / rx), (-sinXAxisRotation * x2 + cosXAxisRotation * y2) * (1.0f / ry) );
        SkPoint delta = point2 - point1;
        float d = delta.x() * delta.x() + delta.y() * delta.y();
        if( d <= 0 )
            return;
        
        float theta1, thetaDelta;
        SkPoint center;
		
        float s = sqrtf( std::max<float>( 1 / d - 0.25f, 0 ) );
        if( sweepFlag == largeArcFlag )
            s = -s;
        
        center = SkPoint::Make( 0.5f * (point1.x() + point2.x()) - delta.y() * s, 0.5f * (point1.y() + point2.y()) + delta.x() * s );
        
        theta1 = atan2f( point1.y() - center.y(), point1.x() - center.x() );
        float theta2 = atan2f( point2.y() - center.y(), point2.x() - center.x() );
        
        thetaDelta = theta2 - theta1;
        if( thetaDelta < 0 && sweepFlag )
            thetaDelta += 2 * (float)M_PI;
        else if( thetaDelta > 0 && ( ! sweepFlag ) )
            thetaDelta -= 2 * (float)M_PI;
        //        path.arcTo(SkRect::MakeLTRB(center.x()-rx, center.y()-ry, center.x()+rx, center.y()+ry), theta1, thetaDelta, true);
        
        // divide the full arc delta into pi/2 arcs and convert those to cubic beziers
        int segments = (int)(ceilf( fabsf(thetaDelta / ( (float)M_PI / 2 )) ) + 1);
        for( int i = 0; i < segments; ++i ) {
            float thetaStart = theta1 + i * thetaDelta / segments;
            float thetaEnd = theta1 + (i + 1) * thetaDelta / segments;
            float t = (4 / 3.0f) * tanf( 0.25f * (thetaEnd - thetaStart) );
            float sinThetaStart = sinf( thetaStart );
            float cosThetaStart = cosf( thetaStart );
            float sinThetaEnd = sinf( thetaEnd );
            float cosThetaEnd = cosf( thetaEnd );
            
            SkPoint startPoint = SkPoint::Make( cosThetaStart - t * sinThetaStart, sinThetaStart + t * cosThetaStart ) + center;
            startPoint = SkPoint::Make( cosXAxisRotation * startPoint.x() * rx - sinXAxisRotation * startPoint.y() * ry, sinXAxisRotation * startPoint.x() * rx + cosXAxisRotation * startPoint.y() * ry );
            SkPoint endPoint = SkPoint::Make(cosThetaEnd, sinThetaEnd ) + center;
            SkPoint transformedEndPoint = SkPoint::Make( cosXAxisRotation * endPoint.x() * rx - sinXAxisRotation * endPoint.y() * ry, sinXAxisRotation * endPoint.x() * rx + cosXAxisRotation * endPoint.y() * ry );
            SkPoint midPoint = endPoint + SkPoint::Make( t * sinThetaEnd, -t * cosThetaEnd );
            midPoint = SkPoint::Make( cosXAxisRotation * midPoint.x() * rx - sinXAxisRotation * midPoint.y() * ry, sinXAxisRotation * midPoint.x() * rx + cosXAxisRotation * midPoint.y() * ry );
            path.cubicTo( startPoint, midPoint, transformedEndPoint );
        }
    }
    
    static const char* getNextPathItem(const char* s, char it[64] )
    {
        int i = 0;
        it[0] = '\0';
        // Skip white spaces and commas
        while( *s && (isspace(*s) || *s == ',') )
            s++;
        if( ! *s )
            return s;
        if( isNumeric(*s) ) {
            while( *s == '-' || *s == '+' ) {
                if( i < 63 )
                    it[i++] = *s;
                s++;
            }
            bool parsingExponent = false;
            while( *s && ( parsingExponent || (*s != '-' && *s != '+')) && isNumeric(*s) ) {
                if( i < 63 )
                    it[i++] = *s;
                if( *s == 'e' || *s == 'E' )
                    parsingExponent = true;
                else
                    parsingExponent = false;
                s++;
            }
            it[i] = '\0';
        }
        else {
            it[0] = *s++;
            it[1] = '\0';
            return s;
        }
        
        return s;
    }
    
    char readNextCommand( const char **sInOut )
    {
        const char *s = *sInOut;
        while( *s && (isspace(*s) || *s == ',') )
            s++;
        *sInOut = s + 1;
        return *s;
    }
    
    bool readFlag( const char **sInOut )
    {
        const char *s = *sInOut;
        while( *s && ( isspace(*s) || *s == ',' || *s == '-' || *s == '+' ) )
            s++;
        *sInOut = s + 1;
        return *s != '0';
    }
    
    bool nextItemIsFloat( const char *s )
    {
        while( *s && (isspace(*s) || *s == ',') )
            s++;
        return isNumeric( *s );
    }
    
    void parsePath( const std::string &p, SkPath* ppath )
    {
        const char* s = p.c_str();
        SkPoint v0, v1, v2;
        SkPoint lastPoint = SkPoint::Make(0, 0), lastPoint2 = SkPoint::Make(0, 0);
        
        SkPath result;
        bool done = false;
        bool firstCmd = true;
        char prevCmd = '\0';
        while( ! done ) {
            char cmd = readNextCommand( &s );
            switch( cmd ) {
                case 'm':
                case 'M':
                    v0.set( parseFloat( &s ), parseFloat( &s ) );
                    if( ( ! firstCmd ) && ( cmd == 'm' ) )
                        v0 += lastPoint;
                    result.moveTo( v0 );
                    lastPoint2 = lastPoint;
                    lastPoint = v0;
                    while( nextItemIsFloat( s ) ) {
                        v0.set( parseFloat( &s ), parseFloat( &s ) );
                        if( cmd == 'm' )
                            v0 += lastPoint;
                        result.lineTo( v0 );
                        lastPoint2 = lastPoint;
                        lastPoint = v0;
                    }
                    break;
                case 'l':
                case 'L':
                    do {
                        v0.set( parseFloat( &s ), parseFloat( &s ) );
                        if( cmd == 'l' )
                            v0 += lastPoint;
                        result.lineTo( v0 );
                        lastPoint2 = lastPoint;
                        lastPoint = v0;
                    } while( nextItemIsFloat( s ) );
                    break;
                case 'H':
                case 'h':
                    do {
                        float x = parseFloat( &s );
                        v0 = SkPoint::Make( ( cmd == 'h' ) ? (lastPoint.x() + x) : x, lastPoint.y() );
                        result.lineTo( v0 );
                        lastPoint2 = lastPoint;
                        lastPoint = v0;
                    } while( nextItemIsFloat( s ) );
                    break;
                case 'V':
                case 'v':
                    do {
                        float y = parseFloat( &s );
                        v0 = SkPoint::Make( lastPoint.x(), ( cmd == 'v' ) ? (lastPoint.y() + y) : (y) );
                        result.lineTo( v0 );
                        lastPoint2 = lastPoint;
                        lastPoint = v0;
                    } while( nextItemIsFloat( s ) );
                    break;
                case 'C':
                case 'c':
                    do {
                        v0.set( parseFloat( &s ), parseFloat( &s ) );
                        v1.set( parseFloat( &s ), parseFloat( &s ) );
                        v2.set( parseFloat( &s ), parseFloat( &s ) );
                        if( cmd == 'c' ) { // relative
                            v0 += lastPoint; v1 += lastPoint; v2 += lastPoint;
                        }
                        result.cubicTo( v0, v1, v2 );
                        lastPoint2 = v1;
                        lastPoint = v2;
                    } while( nextItemIsFloat( s ) );
                    break;
                case 'S':
                case 's':
                    do {
                        if( prevCmd == 's' || prevCmd == 'S' || prevCmd == 'c' || prevCmd == 'C' )
                        {
                            v0 = lastPoint;
                            v0.scale(2.0f);
                            v0 -= lastPoint2;
                        }
                        else
                            v0 = lastPoint;
                        prevCmd = cmd; // set this now in case we loop
                        v1.set( parseFloat( &s ), parseFloat( &s ) );
                        v2.set( parseFloat( &s ), parseFloat( &s ) );
                        if( cmd == 's' ) { // relative
                            v1 += lastPoint; v2 += lastPoint;
                        }
                        result.cubicTo( v0, v1, v2 );
                        lastPoint2 = v1;
                        lastPoint = v2;
                    } while( nextItemIsFloat( s ) );
                    break;
                case 'Q':
                case 'q':
                    do {
                        v0.set( parseFloat( &s ), parseFloat( &s ) );
                        v1.set( parseFloat( &s ), parseFloat( &s ) );
                        if( cmd == 'q' ) { // relative
                            v0 += lastPoint; v1 += lastPoint;
                        }
                        result.quadTo( v0, v1 );
                        lastPoint2 = v0;
                        lastPoint = v1;
                    } while( nextItemIsFloat( s ) );
                    break;
                case 'T':
                case 't':
                    do {
                        if( prevCmd == 't' || prevCmd == 'T' || prevCmd == 'q' || prevCmd == 'Q' )
                        {
                            v0 = lastPoint;
                            v0.scale(2.0f);
                            v0 -= lastPoint2;
                        }
                        else
                            v0 = lastPoint;
                        prevCmd = cmd; // set this now in case we loop
                        v1.set( parseFloat( &s ), parseFloat( &s ) );
                        if( cmd == 't' ) { // relative
                            v1 += lastPoint;
                        }
                        result.quadTo( v0, v1 );
                        lastPoint2 = v0;
                        lastPoint = v1;
                    } while( nextItemIsFloat( s ) );
                    break;
                case 'a':
                case 'A': {
                    do {
                        float ra = parseFloat( &s );
                        float rb = parseFloat( &s );
                        float xAxisRotation = parseFloat( &s ) * (float)M_PI / 180.0f;
                        bool largeArc = readFlag( &s );
                        bool sweepFlag = readFlag( &s );
                        v0.set( parseFloat( &s ), parseFloat( &s ) );
                        if( cmd == 'a' ) { // relative
                            v0 += lastPoint;
                        }
                        ellipticalArc( result, lastPoint.x(), lastPoint.y(), v0.x(), v0.y(), ra, rb, xAxisRotation, largeArc, sweepFlag );
                        lastPoint2 = lastPoint;
                        lastPoint = v0;
                    } while( nextItemIsFloat( s ) );
                }
                    break;
                case 'z':
                case 'Z':
                    result.close();
                    lastPoint2 = lastPoint;
                    result.getLastPt(&lastPoint);
                    break;
                case '\0':
                default: // technically noise at the end of the string is acceptable according to the spec; see W3C_SVG_11/paths-data-18.svg
                    done = true;
                    break;
            }
            firstCmd = false;
            prevCmd = cmd;
        }
        
        ppath->swap(result);
    }
}